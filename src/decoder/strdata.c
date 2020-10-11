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

#include <string.h>
#include "strdata.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

static int strdata_GetBits(StrData *pStrData, long *outword, int out_bitptr)
{
    const int masks[] = { 0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };

    if (out_bitptr == 1)
    {
        if (pStrData->nBitPosition == 0)
        {
            pStrData->nDataByte = pStrData->lDstdata[pStrData->nByteCounter++];

            if (pStrData->nByteCounter > pStrData->nTotalBytes)
            {
                return -1;
            }

            pStrData->nBitPosition = 8;
        }

        pStrData->nBitPosition--;
        *outword = (pStrData->nDataByte >> pStrData->nBitPosition) & 1;

        return 0;
    }

    *outword = 0;

    while (out_bitptr > 0)
    {
        int thisbits, mask, shift;

        if (!pStrData->nBitPosition)
        {
            pStrData->nDataByte = pStrData->lDstdata[pStrData->nByteCounter++];

            if (pStrData->nByteCounter > pStrData->nTotalBytes)
            {
                return -1;
            }

            pStrData->nBitPosition = 8;
        }

        thisbits = MIN(pStrData->nBitPosition, out_bitptr);
        shift = (pStrData->nBitPosition - thisbits);
        mask = masks[thisbits] << shift;
        shift = (out_bitptr - thisbits) - shift;

        if (shift <= 0)
        {
            *outword |= ((pStrData->nDataByte & mask) >> -shift);
        }
        else
        {
            *outword |= ((pStrData->nDataByte & mask) << shift);
        }

        out_bitptr -= thisbits;
        pStrData->nBitPosition -= thisbits;
    }

    return 0;
}

void strdata_GetDstDataPointer(StrData *pStrData, uint8_t** pBuffer)
{
    *pBuffer = pStrData->lDstdata;
}

void strdata_ResetReadingIndex(StrData *pStrData)
{
    pStrData->nBitPosition = 0;
    pStrData->nByteCounter = 0;
    pStrData->nDataByte = 0;
}

void strdata_CreateBuffer(StrData *pStrData, int nSize)
{
    if (nSize > (int)sizeof(pStrData->lDstdata))
    {
        pStrData->nTotalBytes = sizeof(pStrData->lDstdata);
    }
    else
    {
        pStrData->nTotalBytes = nSize;
    }
}

void strdata_DeleteBuffer(StrData *pStrData)
{
    pStrData->nTotalBytes = 0;
    strdata_ResetReadingIndex(pStrData);
}

void strdata_FillBuffer(StrData *pStrData, uint8_t* lBuf, int nSize)
{
    strdata_CreateBuffer(pStrData, nSize);
    memcpy(pStrData->lDstdata, lBuf, pStrData->nTotalBytes);
    strdata_ResetReadingIndex(pStrData);
}

void strdata_GetChrUnsigned(StrData *pStrData, int nLength, uint8_t *pChr)
{
    long tmp = 0;

    if (nLength > 0)
    {
        strdata_GetBits(pStrData, &tmp, nLength);

        *pChr = (unsigned char)tmp;
    }
    else if (nLength == 0)
    {
        *pChr = 0;
    }
    else
    {
        printf("PANIC: Negative number of bits allocated\n");
    }
}

void strdata_GetIntUnsigned(StrData *pStrData, int nLength, int *pChr)
{
    long tmp = 0;

    if (nLength > 0)
    {
        strdata_GetBits(pStrData, &tmp, nLength);

        *pChr = (int)tmp;
    }
    else if (nLength == 0)
    {
        *pChr = 0;
    }
    else
    {
        printf("PANIC: Negative number of bits allocated\n");
    }
}

void strdata_GetIntSigned(StrData *pStrData, int nLength, int *pNum)
{
    long tmp = 0;

    if (nLength > 0)
    {
        strdata_GetBits(pStrData, &tmp, nLength);

        *pNum = (int)tmp;

        if (*pNum >= (1 << (nLength - 1)))
        {
            *pNum -= (1 << nLength);
        }
    }
    else if (nLength == 0)
    {
        *pNum = 0;
    }
    else
    {
        printf("PANIC: Negative number of bits allocated\n");
    }
}

void strdata_GetShortSigned(StrData *pStrData, int nLength, short *pNum)
{
    long tmp = 0;

    if (nLength > 0)
    {
        strdata_GetBits(pStrData, &tmp, nLength);

        *pNum = (short)tmp;

        if (*pNum >= (1 << (nLength - 1)))
        {
            *pNum -= (1 << nLength);
        }
    }
    else if (nLength == 0)
    {
        *pNum = 0;
    }
    else
    {
        printf("PANIC: Negative number of bits allocated\n");
    }
}

int strdata_GetInBitCount(StrData *pStrData)
{
    return pStrData->nByteCounter * 8 - pStrData->nBitPosition;
}
