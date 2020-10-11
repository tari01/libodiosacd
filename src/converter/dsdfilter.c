/*
    Copyright (c) 2015-2020 Robert Tari <robert@tari.in>
    Copyright (c) 2011-2015 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>

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

#include "dsdfilter.h"
#include "memory.h"

void dsdfilter_New(DsdFilter* pDsdFilter)
{
    pDsdFilter->pTables = NULL;
    pDsdFilter->nOrder = 0;
    pDsdFilter->nLength = 0;
    pDsdFilter->nDecimation = 0;
    pDsdFilter->lBuffer = NULL;
    pDsdFilter->nIndex = 0;
}

void dsdfilter_Init(DsdFilter* pDsdFilter, CTable *pTables, int nLength, int nDecimation)
{
    pDsdFilter->pTables = pTables;
    pDsdFilter->nOrder = nLength - 1;
    pDsdFilter->nLength = (nLength + 7) / 8;
    pDsdFilter->nDecimation = nDecimation / 8;
    int buf_size = 2 * pDsdFilter->nLength * sizeof(uint8_t);
    pDsdFilter->lBuffer = (uint8_t*)memAlloc(buf_size);
    memset(pDsdFilter->lBuffer, 0x69, buf_size);
    pDsdFilter->nIndex = 0;
}

void dsdfilter_Free(DsdFilter* pDsdFilter)
{
    if (pDsdFilter->lBuffer)
    {
        memFree(pDsdFilter->lBuffer);
        pDsdFilter->lBuffer = NULL;
    }
}

float dsdfilter_GetDelay(DsdFilter* pDsdFilter)
{
    return (float)pDsdFilter->nOrder / 2 / 8 / pDsdFilter->nDecimation;
}

int dsdfilter_Run(DsdFilter* pDsdFilter, uint8_t *lDsdData, double *lPcmData, int nDsdSamples)
{
    int pcm_samples = nDsdSamples / pDsdFilter->nDecimation;

    for (int sample = 0; sample < pcm_samples; sample++)
    {
        for (int i = 0; i < pDsdFilter->nDecimation; i++)
        {
            pDsdFilter->lBuffer[pDsdFilter->nIndex + pDsdFilter->nLength] = pDsdFilter->lBuffer[pDsdFilter->nIndex] = *(lDsdData++);
            pDsdFilter->nIndex = pDsdFilter->nIndex + 1;
            pDsdFilter->nIndex = pDsdFilter->nIndex % pDsdFilter->nLength;
        }

        lPcmData[sample] = (double)0;

        for (int j = 0; j < pDsdFilter->nLength; j++)
        {
            lPcmData[sample] += pDsdFilter->pTables[j][pDsdFilter->lBuffer[pDsdFilter->nIndex + j]];
        }
    }

    return pcm_samples;
}
