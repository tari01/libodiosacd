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

    Copyright © 2004-2016.

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

#define PBITS 8
#define PSUM (1 << (PBITS))
#define ABITS (PBITS + 4)
#define ONE (1 << ABITS)
#define HALF (1 << (ABITS - 1))

int acdata_GetTableIndex(long nPredVal, int nTableLen)
{
    int j = (nPredVal > 0 ? nPredVal : -nPredVal) >> 3;

    if (j >= nTableLen)
    {
        j = nTableLen - 1;
    }

    return j;
}

void acdata_Init(ACData *pACData, uint8_t* pADataByte, int fs)
{
    pACData->nInit = 0;
    pACData->nA = ONE - 1;
    pACData->nC = 0;

    for (pACData->nBit = 1; pACData->nBit <= ABITS; pACData->nBit++)
    {
        pACData->nC <<= 1;

        if (pACData->nBit < fs)
        {
            pACData->nC |= GET_BIT(pADataByte, pACData->nBit);
        }
    }
}

void acdata_Decode(ACData *pACData, uint8_t* b, int p, uint8_t* pADataByte, int fs)
{
    unsigned int ap;
    unsigned int h;

    ap = ((pACData->nA >> PBITS) | ((pACData->nA >> (PBITS - 1)) & 1)) * p;
    h = pACData->nA - ap;

    if (pACData->nC >= h)
    {
        *b = 0;
        pACData->nC -= h;
        pACData->nA = ap;
    }
    else
    {
        *b = 1;
        pACData->nA = h;
    }

    while (pACData->nA < HALF)
    {
        pACData->nA <<= 1;
        pACData->nC <<= 1;

        if (pACData->nBit < fs)
        {
            pACData->nC |= GET_BIT(pADataByte, pACData->nBit);
        }

        pACData->nBit++;
    }
}

void acdata_Flush(ACData *pACData, uint8_t* b, int p, uint8_t* pADataByte, int fs)
{
    pACData->nInit = 1;

    if (pACData->nBit < fs - 7)
    {
        *b = 0;
    }
    else
    {
        *b = 1;

        while ((pACData->nBit < fs) && (*b == 1))
        {
            if (GET_BIT(pADataByte, pACData->nBit) != 0)
            {
                *b = 1;
            }

            pACData->nBit++;
        }
    }
}
