/*

    MPEG-4 Audio RM Module
    Lossless coding of 1-bit oversampled audio - DST (Direct Stream Transfer)

    This software was originally developed by:

    * Aad Rijnberg
    Philips Digital Systems Laboratories Eindhoven
    <aad.rijnberg@philips.com>

    * Fons Bruekers
    Philips Research Laboratories Eindhoven
    <fons.bruekers@philips.com>

    * Eric Knapen
    Philips Digital Systems Laboratories Eindhoven
    <h.w.m.knapen@philips.com>

    And edited by:

    * Richard Theelen
    Philips Digital Systems Laboratories Eindhoven
    <r.h.m.theelen@philips.com>

    * Maxim V.Anisiutkin
    <maxim.anisiutkin@gmail.com>

    * Robert Tari
    <robert@tari.in>

    in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
    This software module is an implementation of a part of one or more MPEG-4 Audio
    tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
    MPEG-4 Audio standards free licence to this software module or modifications
    thereof for use in hardware or software products claiming conformance to the
    MPEG-4 Audio standards. Those intending to use this software module in hardware
    or software products are advised that this use may infringe existing patents.
    The original developers of this software of this module and their company,
    the subsequent editors and their companies, and ISO/EIC have no liability for
    use of this software module or modifications thereof in an implementation.
    Copyright is not released for non MPEG-4 Audio conforming products. The
    original developer retains full right to use this code for his/her own purpose,
    assign or donate the code to a third party and to inhibit third party from
    using the code for non MPEG-4 Audio conforming products. This copyright notice
    must be included in all copies of derivative works.

    Copyright © 2004-2020.

    This file is part of Odio SACD library.

    Odio SACD library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Odio SACD library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Odio SACD library. If not, see <http://www.gnu.org/licenses/gpl-3.0.txt>.

*/

#include "acdata.h"
#include "decoderbase.h"
#include <string.h>

#define GET_NIBBLE(NibbleBase, NibbleIndex) ((((unsigned char*)NibbleBase)[NibbleIndex >> 1] >> ((NibbleIndex & 1) << 2)) & 0x0f)
#define LT_RUN_FILTER_I(FilterTable, ChannelStatus) \
Predict  = FilterTable[ 0][ChannelStatus[ 0]]; \
Predict += FilterTable[ 1][ChannelStatus[ 1]]; \
Predict += FilterTable[ 2][ChannelStatus[ 2]]; \
Predict += FilterTable[ 3][ChannelStatus[ 3]]; \
Predict += FilterTable[ 4][ChannelStatus[ 4]]; \
Predict += FilterTable[ 5][ChannelStatus[ 5]]; \
Predict += FilterTable[ 6][ChannelStatus[ 6]]; \
Predict += FilterTable[ 7][ChannelStatus[ 7]]; \
Predict += FilterTable[ 8][ChannelStatus[ 8]]; \
Predict += FilterTable[ 9][ChannelStatus[ 9]]; \
Predict += FilterTable[10][ChannelStatus[10]]; \
Predict += FilterTable[11][ChannelStatus[11]]; \
Predict += FilterTable[12][ChannelStatus[12]]; \
Predict += FilterTable[13][ChannelStatus[13]]; \
Predict += FilterTable[14][ChannelStatus[14]]; \
Predict += FilterTable[15][ChannelStatus[15]];

int decoderbase_Init(DecoderBase *pDecoderBase, int nChannels, int nFs44)
{
    pDecoderBase->cFrameHeader.nChannels = nChannels;
    pDecoderBase->cFrameHeader.nMaxFrameLen = (588 * nFs44 / 8);
    pDecoderBase->cFrameHeader.nByteStreamLen = pDecoderBase->cFrameHeader.nMaxFrameLen * pDecoderBase->cFrameHeader.nChannels;
    pDecoderBase->cFrameHeader.nBitStreamLen = pDecoderBase->cFrameHeader.nByteStreamLen * 8;
    pDecoderBase->cFrameHeader.nBitsPerCh = pDecoderBase->cFrameHeader.nMaxFrameLen * 8;
    pDecoderBase->cFrameHeader.nMaxFilters = 2 * pDecoderBase->cFrameHeader.nChannels;
    pDecoderBase->cFrameHeader.nMaxPTables = 2 * pDecoderBase->cFrameHeader.nChannels;
    pDecoderBase->cFrameHeader.nFrame = 0;
    codedtablef_New(&pDecoderBase->cCodedTableF);
    codedtablep_New(&pDecoderBase->cCodedTableP);

    return 0;
}

static void decoderbase_FillTable4Bit(DecoderBase *pDecoderBase, Segment *pSegment, uint8_t Table4Bit[6][75264])
{
    int SegNr;
    int Start;
    int End;
    int8_t Val;

    for (int ChNr = 0; ChNr < pDecoderBase->cFrameHeader.nChannels; ChNr++)
    {
        for (SegNr = 0, Start = 0; SegNr < pSegment->lSegments[ChNr] - 1; SegNr++)
        {
            Val = (int8_t)pSegment->lTable[ChNr][SegNr];
            End = Start + pSegment->nResolution * 8 * pSegment->lLengths[ChNr][SegNr];

            for (int BitNr = Start; BitNr < End; BitNr++)
            {
                uint8_t* p = (uint8_t*)&Table4Bit[ChNr][BitNr / 2];
                int s = (BitNr & 1) << 2;
                *p = ((uint8_t)Val << s) | (*p & (0xf0 >> s));
            }

            Start += pSegment->nResolution * 8 * pSegment->lLengths[ChNr][SegNr];
        }

        Val = (int8_t)pSegment->lTable[ChNr][SegNr];

        for (int BitNr = Start; BitNr < pDecoderBase->cFrameHeader.nBitsPerCh; BitNr++)
        {
            uint8_t* p = (uint8_t*)&Table4Bit[ChNr][BitNr / 2];
            int s = (BitNr & 1) << 2;
            *p = ((uint8_t)Val << s) | (*p & (0xf0 >> s));
        }
    }
}

static void decoderbase_InitCoefTables(DecoderBase *pDecoderBase, int16_t ICoefI[12][16][256])
{
    int FilterNr, FilterLength, TableNr, k, i, j;

    for (FilterNr = 0; FilterNr < pDecoderBase->cFrameHeader.nFilters; FilterNr++)
    {
        FilterLength = pDecoderBase->cFrameHeader.lPredOrder[FilterNr];

        for (TableNr = 0; TableNr < 16; TableNr++)
        {
            k = FilterLength - TableNr * 8;

            if (k > 8)
            {
                k = 8;
            }
            else if (k < 0)
            {
                k = 0;
            }

            for (i = 0; i < 256; i++)
            {
                int cvalue = 0;

                for (j = 0; j < k; j++)
                {
                    cvalue += (((i >> j) & 1) * 2 - 1) * pDecoderBase->cFrameHeader.lICoefA[FilterNr][TableNr * 8 + j];
                }

                ICoefI[FilterNr][TableNr][i] = (int16_t)cvalue;
            }
        }
    }
}

static void decoderbase_InitStatus(DecoderBase *pDecoderBase, uint8_t Status[6][16])
{
    int ChNr, TableNr;

    for (ChNr = 0; ChNr < pDecoderBase->cFrameHeader.nChannels; ChNr++)
    {
        for (TableNr = 0; TableNr < 16; TableNr++)
        {
            Status[ChNr][TableNr] = 0xaa;
        }
    }
}

static int16_t decoderbase_Reverse7LSBs(int16_t c)
{
    const int16_t reverse[128] =
    {
        1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121,
        5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
        3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
        7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127,
        2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122,
        6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126,
        4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124,
        8, 72, 40, 104, 24, 88, 56, 120, 16, 80, 48, 112, 32, 96, 64, 128
    };

    return reverse[(c + (1 << 9)) & 127];
}

int decoderbase_Decode(DecoderBase *pDecoderBase, uint8_t* lDstFrame, int nFrameSize, uint8_t* lDsdFrame)
{
    int rv = 0;
    int ChNr;
    int BitNr;
    uint8_t ACError;
    int nBitsPerCh = pDecoderBase->cFrameHeader.nBitsPerCh;
    int nChannels = pDecoderBase->cFrameHeader.nChannels;
    pDecoderBase->cFrameHeader.nFrame++;
    pDecoderBase->cFrameHeader.nCalcBytes = nFrameSize / 8;
    pDecoderBase->cFrameHeader.nCalcBits = pDecoderBase->cFrameHeader.nCalcBytes * 8;
    rv = decoderbase_Unpack(pDecoderBase, lDstFrame, lDsdFrame);

    if (rv == -1)
    {
        return -1;
    }

    if (pDecoderBase->cFrameHeader.nDstCoded == 1)
    {
        ACData AC;
        int16_t LT_ICoefI[12][16][256];
        uint8_t LT_Status[6][16];

        decoderbase_FillTable4Bit(pDecoderBase, &pDecoderBase->cFrameHeader.cSegmentF, pDecoderBase->cFrameHeader.lFilter4Bit);
        decoderbase_FillTable4Bit(pDecoderBase, &pDecoderBase->cFrameHeader.cSegmentP, pDecoderBase->cFrameHeader.lPTable4Bit);
        decoderbase_InitCoefTables(pDecoderBase, LT_ICoefI);
        decoderbase_InitStatus(pDecoderBase, LT_Status);
        acdata_Init(&AC, pDecoderBase->lADataByte, pDecoderBase->nADataLen);
        acdata_Decode(&AC, &ACError, decoderbase_Reverse7LSBs(pDecoderBase->cFrameHeader.lICoefA[0][0]), pDecoderBase->lADataByte, pDecoderBase->nADataLen);
        memset(lDsdFrame, 0, (nBitsPerCh * nChannels + 7) / 8);

        for (BitNr = 0; BitNr < nBitsPerCh; BitNr++)
        {
            for (ChNr = 0; ChNr < nChannels; ChNr++)
            {
                int16_t Predict;
                uint8_t Residual;
                int16_t BitVal;
                const int FilterNr = GET_NIBBLE(pDecoderBase->cFrameHeader.lFilter4Bit[ChNr], BitNr);
                LT_RUN_FILTER_I(LT_ICoefI[FilterNr], LT_Status[ChNr]);

                if ((pDecoderBase->cFrameHeader.lHalfProbs[ChNr]) && (BitNr < pDecoderBase->cFrameHeader.lHalfBits[ChNr]))
                {
                    acdata_Decode(&AC, &Residual, (1 << 8) / 2, pDecoderBase->lADataByte, pDecoderBase->nADataLen);
                }
                else
                {
                    int PtableNr = GET_NIBBLE(pDecoderBase->cFrameHeader.lPTable4Bit[ChNr], BitNr);
                    int PtableIndex = acdata_GetTableIndex(Predict, pDecoderBase->cFrameHeader.lPTableLengths[PtableNr]);
                    acdata_Decode(&AC, &Residual, pDecoderBase->lPOne[PtableNr][PtableIndex], pDecoderBase->lADataByte, pDecoderBase->nADataLen);
                }

                BitVal = ((((uint16_t)Predict) >> 15) ^ Residual) & 1;
                lDsdFrame[(BitNr >> 3) * nChannels + ChNr] |= (uint8_t)(BitVal << (7 - (BitNr & 7)));
                uint32_t* const st = (uint32_t*)LT_Status[ChNr];
                st[3] = (st[3] << 1) | ((st[2] >> 31) & 1);
                st[2] = (st[2] << 1) | ((st[1] >> 31) & 1);
                st[1] = (st[1] << 1) | ((st[0] >> 31) & 1);
                st[0] = (st[0] << 1) | BitVal;
            }
        }

        acdata_Flush(&AC, &ACError, 0, pDecoderBase->lADataByte, pDecoderBase->nADataLen);

        if (ACError != 1)
        {
            printf("PANIC: Arithmetic decoding error\n");
            rv = -1;
        }
    }

    return rv;
}

int decoderbase_Unpack(DecoderBase *pDecoderBase, uint8_t* lDstFrame, uint8_t* lDsdFrame)
{
    int Dummy;
    int Ready = 0;
    strdata_FillBuffer(&pDecoderBase->cStrData, lDstFrame, pDecoderBase->cFrameHeader.nCalcBytes);
    strdata_GetIntUnsigned(&pDecoderBase->cStrData, 1, &pDecoderBase->cFrameHeader.nDstCoded);

    if (pDecoderBase->cFrameHeader.nDstCoded == 0)
    {
        strdata_GetIntUnsigned(&pDecoderBase->cStrData, 1, &Dummy);
        strdata_GetIntUnsigned(&pDecoderBase->cStrData, 6, &Dummy);

        if (Dummy != 0)
        {
            printf("PANIC: Illegal stuffing pattern in frame %d!\n", pDecoderBase->cFrameHeader.nFrame);

            return -1;
        }

        framereader_ReadDsdFrame(&pDecoderBase->cStrData, pDecoderBase->cFrameHeader.nMaxFrameLen, pDecoderBase->cFrameHeader.nChannels, lDsdFrame);
    }
    else
    {
        framereader_ReadSegmentData(&pDecoderBase->cStrData, &pDecoderBase->cFrameHeader);
        framereader_ReadMappingData(&pDecoderBase->cStrData, &pDecoderBase->cFrameHeader);
        framereader_ReadFilterCoefSets(&pDecoderBase->cStrData, pDecoderBase->cFrameHeader.nChannels, &pDecoderBase->cFrameHeader, &pDecoderBase->cCodedTableF);
        framereader_ReadProbabilityTables(&pDecoderBase->cStrData, &pDecoderBase->cFrameHeader, &pDecoderBase->cCodedTableP, pDecoderBase->lPOne);
        pDecoderBase->nADataLen = pDecoderBase->cFrameHeader.nCalcBits - strdata_GetInBitCount(&pDecoderBase->cStrData);
        framereader_ReadArithmeticCodedData(&pDecoderBase->cStrData, pDecoderBase->nADataLen, pDecoderBase->lADataByte);

        if (pDecoderBase->nADataLen > 0 && GET_BIT(pDecoderBase->lADataByte, 0) != 0)
        {
            printf("PANIC: Illegal arithmetic code in frame %d!", pDecoderBase->cFrameHeader.nFrame);
            return -1;
        }
    }

    return Ready;
}
