
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

#include "converterbase.h"
#include "memory.h"

static void converterbase_FreePcmTemp1(ConverterBase *pConverterBase)
{
    memFree(pConverterBase->lPcmTemp1);
    pConverterBase->lPcmTemp1 = NULL;
}

static void converterbase_FreePcmTemp2(ConverterBase *pConverterBase)
{
    memFree(pConverterBase->lPcmTemp2);
    pConverterBase->lPcmTemp2 = NULL;
}

static void converterbase_AllocPcmTemp1(ConverterBase *pConverterBase, int pcm_samples)
{
    converterbase_FreePcmTemp1(pConverterBase);
    pConverterBase->lPcmTemp1 = (double*)memAlloc(pcm_samples * sizeof(double));
}

static void converterbase_AllocPcmTemp2(ConverterBase *pConverterBase, int pcm_samples)
{
    converterbase_FreePcmTemp2(pConverterBase);
    pConverterBase->lPcmTemp2 = (double*)memAlloc(pcm_samples * sizeof(double));
}

ConverterBase *converterbase_New()
{
    ConverterBase *pConverterBase = malloc(sizeof(ConverterBase));
    pConverterBase->lPcmTemp1 = NULL;
    pConverterBase->lPcmTemp2 = NULL;
    dsdfilter_New(&pConverterBase->cDsdFilter);
    pcmfilter_New(&pConverterBase->cPcmFilter1A);
    pcmfilter_New(&pConverterBase->cPcmFilter1B);
    pcmfilter_New(&pConverterBase->cPcmFilter1C);
    pcmfilter_New(&pConverterBase->cPcmFilter1D);
    pcmfilter_New(&pConverterBase->cPcmFilter2);

    return pConverterBase;
}

void converterbase_Free(ConverterBase *pConverterBase)
{
    converterbase_FreePcmTemp1(pConverterBase);
    converterbase_FreePcmTemp2(pConverterBase);
    dsdfilter_Free(&pConverterBase->cDsdFilter);
    pcmfilter_Free(&pConverterBase->cPcmFilter1A);
    pcmfilter_Free(&pConverterBase->cPcmFilter1B);
    pcmfilter_Free(&pConverterBase->cPcmFilter1C);
    pcmfilter_Free(&pConverterBase->cPcmFilter1D);
    pcmfilter_Free(&pConverterBase->cPcmFilter2);
    free(pConverterBase);
}

float converterbase_GetDelay(ConverterBase *pConverterBase)
{
    return pConverterBase->fDelay;
}

void converterbase_Init(ConverterBase *pConverterBase, FilterSetup *flt_setup, int dsd_samples, int nDecimation)
{
    pConverterBase->nDecimation = nDecimation;

    if (nDecimation == 512)
    {
        converterbase_AllocPcmTemp1(pConverterBase, dsd_samples / 2);
        converterbase_AllocPcmTemp2(pConverterBase, dsd_samples / 4);
        dsdfilter_Init(&pConverterBase->cDsdFilter, filtersetup_GetTables116(flt_setup), 160, 16);
        pcmfilter_Init(&pConverterBase->cPcmFilter1A, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter1B, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter1C, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter1D, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter2, filtersetup_GetCoefs32(flt_setup), 151, 2);
        pConverterBase->fDelay = (((dsdfilter_GetDelay(&pConverterBase->cDsdFilter) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1A ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1A)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1B ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1B)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1C ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1C)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter2) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter2);
    }
    else if (nDecimation == 256)
    {
        converterbase_AllocPcmTemp1(pConverterBase, dsd_samples / 2);
        converterbase_AllocPcmTemp2(pConverterBase, dsd_samples / 4);
        dsdfilter_Init(&pConverterBase->cDsdFilter, filtersetup_GetTables116(flt_setup), 160, 16);
        pcmfilter_Init(&pConverterBase->cPcmFilter1A, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter1B, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter1C, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter2, filtersetup_GetCoefs32(flt_setup), 151, 2);
        pConverterBase->fDelay = (((dsdfilter_GetDelay(&pConverterBase->cDsdFilter) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1A ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1A)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1B ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1B)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1C ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1C)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter2) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter2);
    }
    else if (nDecimation == 128)
    {
        converterbase_AllocPcmTemp1(pConverterBase, dsd_samples / 2);
        converterbase_AllocPcmTemp2(pConverterBase, dsd_samples / 4);
        dsdfilter_Init(&pConverterBase->cDsdFilter, filtersetup_GetTables116(flt_setup), 160, 16);
        pcmfilter_Init(&pConverterBase->cPcmFilter1A, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter1B, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter2, filtersetup_GetCoefs32(flt_setup), 151, 2);
        pConverterBase->fDelay = ((dsdfilter_GetDelay(&pConverterBase->cDsdFilter) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1A ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1A)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1B ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1B)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter2) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter2);
    }
    else if (nDecimation == 64)
    {
        converterbase_AllocPcmTemp1(pConverterBase, dsd_samples / 2);
        converterbase_AllocPcmTemp2(pConverterBase, dsd_samples / 4);
        dsdfilter_Init(&pConverterBase->cDsdFilter, filtersetup_GetTables116(flt_setup), 160, 16);
        pcmfilter_Init(&pConverterBase->cPcmFilter1A, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter2, filtersetup_GetCoefs32(flt_setup), 151, 2);
        pConverterBase->fDelay = (dsdfilter_GetDelay(&pConverterBase->cDsdFilter) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1A ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1A)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter2) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter2);
    }
    else if (nDecimation == 32)
    {
        converterbase_AllocPcmTemp1(pConverterBase, dsd_samples);
        converterbase_AllocPcmTemp2(pConverterBase, dsd_samples / 2);
        dsdfilter_Init(&pConverterBase->cDsdFilter, filtersetup_GetTables18(flt_setup), 80, 8);
        pcmfilter_Init(&pConverterBase->cPcmFilter1A, filtersetup_GetCoefs22(flt_setup), 27, 2);
        pcmfilter_Init(&pConverterBase->cPcmFilter2, filtersetup_GetCoefs32(flt_setup), 151, 2);
        pConverterBase->fDelay = (dsdfilter_GetDelay(&pConverterBase->cDsdFilter) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter1A ) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter1A)) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter2) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter2);
    }
    else if (nDecimation == 16)
    {
        converterbase_AllocPcmTemp1(pConverterBase, dsd_samples);
        dsdfilter_Init(&pConverterBase->cDsdFilter, filtersetup_GetTables18(flt_setup), 80, 8);
        pcmfilter_Init(&pConverterBase->cPcmFilter2, filtersetup_GetCoefs32(flt_setup), 151, 2);
        pConverterBase->fDelay = dsdfilter_GetDelay(&pConverterBase->cDsdFilter) / pcmfilter_GetDecimation(&pConverterBase->cPcmFilter2) + pcmfilter_GetDelay(&pConverterBase->cPcmFilter2);
    }
    else if (nDecimation == 8)
    {
        dsdfilter_Init(&pConverterBase->cDsdFilter, filtersetup_GetTables18(flt_setup), 80, 8);
        pConverterBase->fDelay = dsdfilter_GetDelay(&pConverterBase->cDsdFilter);
    }
}

int converterbase_Convert(ConverterBase *pConverterBase, uint8_t *lDsdData, double *pcm_data, int dsd_samples)
{
    int pcm_samples = 0;

    if (pConverterBase->nDecimation == 512)
    {
        pcm_samples = dsdfilter_Run(&pConverterBase->cDsdFilter, lDsdData, pConverterBase->lPcmTemp1, dsd_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1A, pConverterBase->lPcmTemp1, pConverterBase->lPcmTemp2, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1B, pConverterBase->lPcmTemp2, pConverterBase->lPcmTemp1, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1C, pConverterBase->lPcmTemp1, pConverterBase->lPcmTemp2, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1D, pConverterBase->lPcmTemp2, pConverterBase->lPcmTemp1, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter2, pConverterBase->lPcmTemp1, pcm_data, pcm_samples);
    }
    else if (pConverterBase->nDecimation == 256)
    {
        pcm_samples = dsdfilter_Run(&pConverterBase->cDsdFilter, lDsdData, pConverterBase->lPcmTemp1, dsd_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1A, pConverterBase->lPcmTemp1, pConverterBase->lPcmTemp2, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1B, pConverterBase->lPcmTemp2, pConverterBase->lPcmTemp1, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1C, pConverterBase->lPcmTemp1, pConverterBase->lPcmTemp2, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter2, pConverterBase->lPcmTemp2, pcm_data, pcm_samples);
    }
    else if (pConverterBase->nDecimation == 128)
    {
        pcm_samples = dsdfilter_Run(&pConverterBase->cDsdFilter, lDsdData, pConverterBase->lPcmTemp1, dsd_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1A, pConverterBase->lPcmTemp1, pConverterBase->lPcmTemp2, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1B, pConverterBase->lPcmTemp2, pConverterBase->lPcmTemp1, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter2, pConverterBase->lPcmTemp1, pcm_data, pcm_samples);
    }
    else if (pConverterBase->nDecimation == 64)
    {
        pcm_samples = dsdfilter_Run(&pConverterBase->cDsdFilter, lDsdData, pConverterBase->lPcmTemp1, dsd_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1A, pConverterBase->lPcmTemp1, pConverterBase->lPcmTemp2, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter2, pConverterBase->lPcmTemp2, pcm_data, pcm_samples);
    }
    else if (pConverterBase->nDecimation == 32)
    {
        pcm_samples = dsdfilter_Run(&pConverterBase->cDsdFilter, lDsdData, pConverterBase->lPcmTemp1, dsd_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter1A, pConverterBase->lPcmTemp1, pConverterBase->lPcmTemp2, pcm_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter2, pConverterBase->lPcmTemp2, pcm_data, pcm_samples);
    }
    else if (pConverterBase->nDecimation == 16)
    {
        pcm_samples = dsdfilter_Run(&pConverterBase->cDsdFilter, lDsdData, pConverterBase->lPcmTemp1, dsd_samples);
        pcm_samples = pcmfilter_Run(&pConverterBase->cPcmFilter2, pConverterBase->lPcmTemp1, pcm_data, pcm_samples);
    }
    else if (pConverterBase->nDecimation == 8)
    {
        pcm_samples = dsdfilter_Run(&pConverterBase->cDsdFilter, lDsdData, pcm_data, dsd_samples);
    }

    return pcm_samples;
}
