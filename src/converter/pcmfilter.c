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

#include "pcmfilter.h"
#include "memory.h"

void pcmfilter_New(PcmFilter *pPcmFilter)
{
    pPcmFilter->lCoefs = NULL;
    pPcmFilter->nOrder = 0;
    pPcmFilter->nLength = 0;
    pPcmFilter->nDecimation = 0;
    pPcmFilter->lBuffer = NULL;
    pPcmFilter->nIndex = 0;
}

void pcmfilter_Init(PcmFilter *pPcmFilter, double *lCoefs, int nLength, int nDecimation)
{
    pPcmFilter->lCoefs = lCoefs;
    pPcmFilter->nOrder = nLength - 1;
    pPcmFilter->nLength = nLength;
    pPcmFilter->nDecimation = nDecimation;
    int buf_size = 2 * pPcmFilter->nLength * sizeof(double);
    pPcmFilter->lBuffer = (double*)memAlloc(buf_size);
    memset(pPcmFilter->lBuffer, 0, buf_size);
    pPcmFilter->nIndex = 0;
}

void pcmfilter_Free(PcmFilter *pPcmFilter)
{
    if (pPcmFilter->lBuffer)
    {
        memFree(pPcmFilter->lBuffer);
        pPcmFilter->lBuffer = NULL;
    }
}

int pcmfilter_GetDecimation(PcmFilter *pPcmFilter)
{
    return pPcmFilter->nDecimation;
}

float pcmfilter_GetDelay(PcmFilter *pPcmFilter)
{
    return (float)pPcmFilter->nOrder / 2 / pPcmFilter->nDecimation;
}

int pcmfilter_Run(PcmFilter *pPcmFilter, double *lPcmData, double *lOutData, int nPcmSamples)
{
    int out_samples = nPcmSamples / pPcmFilter->nDecimation;

    for (int sample = 0; sample < out_samples; sample++)
    {
        for (int i = 0; i < pPcmFilter->nDecimation; i++)
        {
            pPcmFilter->lBuffer[pPcmFilter->nIndex + pPcmFilter->nLength] = pPcmFilter->lBuffer[pPcmFilter->nIndex] = *(lPcmData++);
            pPcmFilter->nIndex = pPcmFilter->nIndex + 1;
            pPcmFilter->nIndex = pPcmFilter->nIndex % pPcmFilter->nLength;
        }

        lOutData[sample] = (double)0;

        for (int j = 0; j < pPcmFilter->nLength; j++)
        {
            lOutData[sample] += pPcmFilter->lCoefs[j] * pPcmFilter->lBuffer[pPcmFilter->nIndex + j];
        }
    }

    return out_samples;
}
