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

    Copyright © 2004-2021.

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

#include "framereader.h"

int framereader_Log2RoundUp(long x)
{
    int y = 0;

    while (x >= (1 << y))
    {
        y++;
    }

    return y;
}

int framereader_RiceDecode(StrData *pStrData, int m)
{
    int LSBs;
    int Nr;
    int RLBit;
    int RunLength;
    int Sign;

    RunLength = 0;

    do
    {
        strdata_GetIntUnsigned(pStrData, 1, &RLBit);
        RunLength += (1 - RLBit);

    } while (!RLBit);

    strdata_GetIntUnsigned(pStrData, m, &LSBs);
    Nr = (RunLength << m) + LSBs;

    if (Nr != 0)
    {
        strdata_GetIntUnsigned(pStrData, 1, &Sign);

        if (Sign)
        {
            Nr = -Nr;
        }
    }

    return Nr;
}

void framereader_ReadDsdFrame(StrData *pStrData, long nMaxFrameLen, int nChannels, uint8_t* lDsdFrame)
{
    int ByteMax = nMaxFrameLen * nChannels;

    for (int ByteNr = 0; ByteNr < ByteMax; ByteNr++)
    {
        strdata_GetChrUnsigned(pStrData, 8, &lDsdFrame[ByteNr]);
    }
}

void framereader_ReadTableSegmentData(StrData *pStrData, int nChannels, int FrameLen, int MaxNrOfSegs, int MinSegLen, Segment *S, int *SameSegAllCh)
{
    int ChNr = 0;
    int DefinedBits = 0;
    bool ResolRead = false;
    int SegNr = 0;
    int MaxSegSize;
    int NrOfBits;
    int EndOfChannel;

    MaxSegSize = FrameLen - MinSegLen / 8;
    strdata_GetIntUnsigned(pStrData, 1, SameSegAllCh);

    if (*SameSegAllCh)
    {
        strdata_GetIntUnsigned(pStrData, 1, &EndOfChannel);

        while (!EndOfChannel)
        {
            if (SegNr >= MaxNrOfSegs)
            {
                printf("PANIC: Too many segments for this channel\n");

                return;
            }

            if (!ResolRead)
            {
                NrOfBits = framereader_Log2RoundUp(FrameLen - MinSegLen / 8);
                strdata_GetIntUnsigned(pStrData, NrOfBits, &S->nResolution);

                if ((S->nResolution == 0) || (S->nResolution > FrameLen - MinSegLen / 8))
                {
                    printf("PANIC: Invalid segment resolution\n");

                    return;
                }

                ResolRead = true;
            }

            NrOfBits = framereader_Log2RoundUp(MaxSegSize / S->nResolution);

            strdata_GetIntUnsigned(pStrData, NrOfBits, &S->lLengths[0][SegNr]);

            if ((S->nResolution * 8 * S->lLengths[0][SegNr] < MinSegLen) || (S->nResolution * 8 * S->lLengths[0][SegNr] > FrameLen * 8 - DefinedBits - MinSegLen))
            {
                printf("PANIC: Invalid segment length\n");

                return;
            }

            DefinedBits += S->nResolution * 8 * S->lLengths[0][SegNr];
            MaxSegSize -= S->nResolution * S->lLengths[0][SegNr];
            SegNr++;

            strdata_GetIntUnsigned(pStrData, 1, &EndOfChannel);
        }

        S->lSegments[0] = SegNr + 1;
        S->lLengths[0][SegNr] = 0;

        for (ChNr = 1; ChNr < nChannels; ChNr++)
        {
            S->lSegments[ChNr] = S->lSegments[0];

            for (SegNr = 0; SegNr < S->lSegments[0]; SegNr++)
            {
                S->lLengths[ChNr][SegNr] = S->lLengths[0][SegNr];
            }
        }
    }
    else
    {
        while (ChNr < nChannels)
        {
            if (SegNr >= MaxNrOfSegs)
            {
                printf("PANIC: Too many segments for this channel\n");

                return;
            }

            strdata_GetIntUnsigned(pStrData, 1, &EndOfChannel);

            if (!EndOfChannel)
            {
                if (!ResolRead)
                {
                    NrOfBits = framereader_Log2RoundUp(FrameLen - MinSegLen / 8);
                    strdata_GetIntUnsigned(pStrData, NrOfBits, &S->nResolution);

                    if ((S->nResolution == 0) || (S->nResolution > FrameLen - MinSegLen / 8))
                    {
                        printf("PANIC: Invalid segment resolution\n");

                        return;
                    }

                    ResolRead = true;
                }

                NrOfBits = framereader_Log2RoundUp(MaxSegSize / S->nResolution);
                strdata_GetIntUnsigned(pStrData, NrOfBits, &S->lLengths[ChNr][SegNr]);

                if ((S->nResolution * 8 * S->lLengths[ChNr][SegNr] < MinSegLen) || (S->nResolution * 8 * S->lLengths[ChNr][SegNr] > FrameLen * 8 - DefinedBits - MinSegLen))
                {
                    printf("PANIC: Invalid segment length\n");

                    return;
                }

                DefinedBits += S->nResolution * 8 * S->lLengths[ChNr][SegNr];
                MaxSegSize -= S->nResolution * S->lLengths[ChNr][SegNr];
                SegNr++;
            }
            else
            {
                S->lSegments[ChNr] = SegNr + 1;
                S->lLengths[ChNr][SegNr] = 0;
                SegNr = 0;
                DefinedBits = 0;
                MaxSegSize = FrameLen - MinSegLen / 8;
                ChNr++;
            }
        }
    }

    if (!ResolRead)
    {
        S->nResolution = 1;
    }
}

void framereader_CopySegmentData(FrameHeader *pFrameHeader)
{
    pFrameHeader->cSegmentP.nResolution = pFrameHeader->cSegmentF.nResolution;
    pFrameHeader->nPSameSegAllCh = 1;

    for (int ChNr = 0; ChNr < pFrameHeader->nChannels; ChNr++)
    {
        pFrameHeader->cSegmentP.lSegments[ChNr] = pFrameHeader->cSegmentF.lSegments[ChNr];

        if (pFrameHeader->cSegmentP.lSegments[ChNr] > 8)
        {
            printf("PANIC: Too many segments\n");

            return;
        }

        if (pFrameHeader->cSegmentP.lSegments[ChNr] != pFrameHeader->cSegmentP.lSegments[0])
        {
            pFrameHeader->nPSameSegAllCh = 0;
        }

        for (int SegNr = 0; SegNr < pFrameHeader->cSegmentF.lSegments[ChNr]; SegNr++)
        {
            pFrameHeader->cSegmentP.lLengths[ChNr][SegNr] = pFrameHeader->cSegmentF.lLengths[ChNr][SegNr];

            if ((pFrameHeader->cSegmentP.lLengths[ChNr][SegNr] != 0) &&   (pFrameHeader->cSegmentP.nResolution * 8 * pFrameHeader->cSegmentP.lLengths[ChNr][SegNr] < 32))
            {
                printf("PANIC: Invalid segment length\n");

                return;
            }

            if (pFrameHeader->cSegmentP.lLengths[ChNr][SegNr] != pFrameHeader->cSegmentP.lLengths[0][SegNr])
            {
                pFrameHeader->nPSameSegAllCh = 0;
            }
        }
    }
}

void framereader_ReadSegmentData(StrData *pStrData, FrameHeader *pFrameHeader)
{
    strdata_GetIntUnsigned(pStrData, 1, &pFrameHeader->nPSameSegAsF);
    framereader_ReadTableSegmentData(pStrData, pFrameHeader->nChannels, pFrameHeader->nMaxFrameLen, 4, 1024, &pFrameHeader->cSegmentF, &pFrameHeader->nFSameSegAllCh);

    if (pFrameHeader->nPSameSegAsF == 1)
    {
        framereader_CopySegmentData(pFrameHeader);
    }
    else
    {
        framereader_ReadTableSegmentData(pStrData, pFrameHeader->nChannels, pFrameHeader->nMaxFrameLen, 8, 32, &pFrameHeader->cSegmentP, &pFrameHeader->nPSameSegAllCh);
    }
}

void framereader_ReadTableMappingData(StrData *pStrData, int nChannels, int MaxNrOfTables, Segment *S, int *NrOfTables, int *SameMapAllCh)
{
    int CountTables = 1;
    int NrOfBits = 1;
    S->lTable[0][0] = 0;

    strdata_GetIntUnsigned(pStrData, 1, SameMapAllCh);

    if (*SameMapAllCh)
    {
        for (int SegNr = 1; SegNr < S->lSegments[0]; SegNr++)
        {
            NrOfBits = framereader_Log2RoundUp(CountTables);
            strdata_GetIntUnsigned(pStrData, NrOfBits, &S->lTable[0][SegNr]);

            if (S->lTable[0][SegNr] == CountTables)
            {
                CountTables++;
            }
            else if (S->lTable[0][SegNr] > CountTables)
            {
                printf("PANIC: Invalid table number for segment\n");

                return;
            }
        }

        for (int ChNr = 1; ChNr < nChannels; ChNr++)
        {
            if (S->lSegments[ChNr] != S->lSegments[0])
            {
                printf("PANIC: Mapping can't be the same for all channels\n");

                return;
            }

            for (int SegNr = 0; SegNr < S->lSegments[0]; SegNr++)
            {
                S->lTable[ChNr][SegNr] = S->lTable[0][SegNr];
            }
        }
    }
    else
    {
        for (int ChNr = 0; ChNr < nChannels; ChNr++)
        {
            for (int SegNr = 0; SegNr < S->lSegments[ChNr]; SegNr++)
            {
                if ((ChNr != 0) || (SegNr != 0))
                {
                    NrOfBits = framereader_Log2RoundUp(CountTables);
                    strdata_GetIntUnsigned(pStrData, NrOfBits, &S->lTable[ChNr][SegNr]);

                    if (S->lTable[ChNr][SegNr] == CountTables)
                    {
                        CountTables++;
                    }
                    else if (S->lTable[ChNr][SegNr] > CountTables)
                    {
                        printf("PANIC: Invalid table number for segment\n");

                        return;
                    }
                }
            }
        }
    }

    if (CountTables > MaxNrOfTables)
    {
        printf("PANIC: Too many tables for this frame\n");

        return;
    }

    *NrOfTables = CountTables;
}

void framereader_CopyMappingData(FrameHeader *pFrameHeader)
{
    pFrameHeader->nPSameMapAllCh = 1;

    for (int ChNr = 0; ChNr < pFrameHeader->nChannels; ChNr++)
    {
        if (pFrameHeader->cSegmentP.lSegments[ChNr] == pFrameHeader->cSegmentF.lSegments[ChNr])
        {
            for (int SegNr = 0; SegNr < pFrameHeader->cSegmentF.lSegments[ChNr]; SegNr++)
            {
                pFrameHeader->cSegmentP.lTable[ChNr][SegNr] = pFrameHeader->cSegmentF.lTable[ChNr][SegNr];

                if (pFrameHeader->cSegmentP.lTable[ChNr][SegNr] != pFrameHeader->cSegmentP.lTable[0][SegNr])
                {
                    pFrameHeader->nPSameMapAllCh = 0;
                }
            }
        }
        else
        {
            printf("PANIC: Not the same number of segments for filters and Ptables\n");

            return;
        }
    }

    pFrameHeader->nPtables = pFrameHeader->nFilters;

    if (pFrameHeader->nPtables > pFrameHeader->nMaxPTables)
    {
        printf("PANIC: Too many tables for this frame\n");

        return;
    }
}

void framereader_ReadMappingData(StrData *pStrData, FrameHeader *pFrameHeader)
{
    strdata_GetIntUnsigned(pStrData, 1, &pFrameHeader->nPSameMapAsF);
    framereader_ReadTableMappingData(pStrData, pFrameHeader->nChannels, pFrameHeader->nMaxFilters, &pFrameHeader->cSegmentF, &pFrameHeader->nFilters, &pFrameHeader->nFSameMapAllCh);

    if (pFrameHeader->nPSameMapAsF == 1)
    {
        framereader_CopyMappingData(pFrameHeader);
    }
    else
    {
        framereader_ReadTableMappingData(pStrData, pFrameHeader->nChannels, pFrameHeader->nMaxPTables, &pFrameHeader->cSegmentP, &pFrameHeader->nPtables, &pFrameHeader->nPSameMapAllCh);
    }

    for (int i = 0; i < pFrameHeader->nChannels; i++)
    {
        strdata_GetIntUnsigned(pStrData, 1, &pFrameHeader->lHalfProbs[i]);
    }
}

void framereader_ReadFilterCoefSets(StrData *pStrData, int nChannels, FrameHeader *pFrameHeader, CodedTableF *pCodedTableF)
{
    for (int FilterNr = 0; FilterNr < pFrameHeader->nFilters; FilterNr++)
    {
        strdata_GetIntUnsigned(pStrData, 7, &pFrameHeader->lPredOrder[FilterNr]);
        pFrameHeader->lPredOrder[FilterNr]++;
        strdata_GetIntUnsigned(pStrData, 1, &pCodedTableF->lCoded[FilterNr]);

        if (!pCodedTableF->lCoded[FilterNr])
        {
            pCodedTableF->lBestMethod[FilterNr] = -1;

            for (int CoefNr = 0; CoefNr < pFrameHeader->lPredOrder[FilterNr]; CoefNr++)
            {
                strdata_GetShortSigned(pStrData, 9, &pFrameHeader->lICoefA[FilterNr][CoefNr]);
            }
        }
        else
        {
            strdata_GetIntUnsigned(pStrData, 2, &pCodedTableF->lBestMethod[FilterNr]);

            int bestmethod = pCodedTableF->lBestMethod[FilterNr];

            if (pCodedTableF->lPredOrder[bestmethod] >= pFrameHeader->lPredOrder[FilterNr])
            {
                printf("PANIC: Invalid coefficient coding method\n");

                return;
            }

            for (int CoefNr = 0; CoefNr < pCodedTableF->lPredOrder[bestmethod]; CoefNr++)
            {
                strdata_GetShortSigned(pStrData, 9, &pFrameHeader->lICoefA[FilterNr][CoefNr]);
            }

            strdata_GetIntUnsigned(pStrData, 3, &pCodedTableF->lM[FilterNr][bestmethod]);

            for (int CoefNr = pCodedTableF->lPredOrder[bestmethod]; CoefNr < pFrameHeader->lPredOrder[FilterNr]; CoefNr++)
            {
                int x = 0;
                int c;

                for (int TapNr = 0; TapNr < pCodedTableF->lPredOrder[bestmethod]; TapNr++)
                {
                    x += pCodedTableF->lPredCoef[bestmethod][TapNr] * pFrameHeader->lICoefA[FilterNr][CoefNr - TapNr - 1];
                }

                if (x >= 0)
                {
                    c = framereader_RiceDecode(pStrData, pCodedTableF->lM[FilterNr][bestmethod]) - (x + 4) / 8;
                }
                else
                {
                    c = framereader_RiceDecode(pStrData, pCodedTableF->lM[FilterNr][bestmethod]) + (-x + 3) / 8;
                }

                if ((c < -(1 << 8)) || (c >= (1 << 8)))
                {
                    printf("PANIC: Filter coefficient out of range\n");

                    return;
                }
                else
                {
                    pFrameHeader->lICoefA[FilterNr][CoefNr] = (int16_t)c;
                }
            }
        }
    }

    for (int ChNr = 0; ChNr < nChannels; ChNr++)
    {
        pFrameHeader->lHalfBits[ChNr] = pFrameHeader->lPredOrder[pFrameHeader->cSegmentF.lTable[ChNr][0]];
    }
}

void framereader_ReadProbabilityTables(StrData *pStrData, FrameHeader *pFrameHeader, CodedTableP *pCodedTableP, int lPOne[12][1 << 6])
{
    for (int PtableNr = 0; PtableNr < pFrameHeader->nPtables; PtableNr++)
    {
        strdata_GetIntUnsigned(pStrData, 6, &pFrameHeader->lPTableLengths[PtableNr]);
        pFrameHeader->lPTableLengths[PtableNr]++;

        if (pFrameHeader->lPTableLengths[PtableNr] > 1)
        {
            strdata_GetIntUnsigned(pStrData, 1, &pCodedTableP->lCoded[PtableNr]);

            if (!pCodedTableP->lCoded[PtableNr])
            {
                pCodedTableP->lBestMethod[PtableNr] = -1;

                for (int EntryNr = 0; EntryNr < pFrameHeader->lPTableLengths[PtableNr]; EntryNr++)
                {
                    strdata_GetIntUnsigned(pStrData, 7, &lPOne[PtableNr][EntryNr]);
                    lPOne[PtableNr][EntryNr]++;
                }
            }
            else
            {
                strdata_GetIntUnsigned(pStrData, 2, &pCodedTableP->lBestMethod[PtableNr]);

                int bestmethod = pCodedTableP->lBestMethod[PtableNr];

                if (pCodedTableP->lPredOrder[bestmethod] >= pFrameHeader->lPTableLengths[PtableNr])
                {
                    printf("PANIC: Invalid Ptable coding method\n");

                    return;
                }

                for (int EntryNr = 0; EntryNr < pCodedTableP->lPredOrder[bestmethod]; EntryNr++)
                {
                    strdata_GetIntUnsigned(pStrData, 7, &lPOne[PtableNr][EntryNr]);
                    lPOne[PtableNr][EntryNr]++;
                }

                strdata_GetIntUnsigned(pStrData, 3, &pCodedTableP->lM[PtableNr][bestmethod]);

                for (int EntryNr = pCodedTableP->lPredOrder[bestmethod]; EntryNr < pFrameHeader->lPTableLengths[PtableNr]; EntryNr++)
                {
                    int x = 0;
                    int c;

                    for (int TapNr = 0; TapNr < pCodedTableP->lPredOrder[bestmethod]; TapNr++)
                    {
                        x += pCodedTableP->lPredCoef[bestmethod][TapNr] * lPOne[PtableNr][EntryNr - TapNr - 1];
                    }

                    if (x >= 0)
                    {
                        c = framereader_RiceDecode(pStrData, pCodedTableP->lM[PtableNr][bestmethod]) - (x + 4) / 8;
                    }
                    else
                    {
                        c = framereader_RiceDecode(pStrData, pCodedTableP->lM[PtableNr][bestmethod]) + (-x + 3) / 8;
                    }

                    if ((c < 1) || (c > (1 << 7)))
                    {
                        printf("PANIC: Ptable entry out of range\n");

                        return;
                    }
                    else
                    {
                        lPOne[PtableNr][EntryNr] = c;
                    }
                }
            }
        }
        else
        {
            lPOne[PtableNr][0] = 128;
            pCodedTableP->lBestMethod[PtableNr] = -1;
        }
    }
}

void framereader_ReadArithmeticCodedData(StrData *pStrData, int nADataLen, uint8_t* lADataByte)
{
    for (int j = 0; j < (nADataLen >> 3); j++)
    {
        uint8_t v;

        strdata_GetChrUnsigned(pStrData, 8, &v);
        lADataByte[j] = v;
    }

    uint8_t Val = 0;

    for (int j = nADataLen & ~7; j < nADataLen; j++)
    {
        uint8_t v;

        strdata_GetChrUnsigned(pStrData, 1, &v);

        Val |= v << (7 - (j & 7));

        if (j == nADataLen - 1)
        {
            lADataByte[j >> 3] = Val;
            Val = 0;
        }
    }
}
