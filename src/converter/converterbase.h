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

#ifndef CONVERTERBASE_H
#define CONVERTERBASE_H

#include "filtersetup.h"
#include "dsdfilter.h"
#include "pcmfilter.h"

typedef struct
{
    int nFrameRate;
    int nDsdSampleRate;
    int nPcmSampleRate;
    float fDelay;
    double *lPcmTemp1;
    double *lPcmTemp2;
    DsdFilter cDsdFilter;
    PcmFilter cPcmFilter1A;
    PcmFilter cPcmFilter1B;
    PcmFilter cPcmFilter1C;
    PcmFilter cPcmFilter1D;
    PcmFilter cPcmFilter2;
    int nDecimation;

} ConverterBase;

ConverterBase *converterbase_New();
void converterbase_Free(ConverterBase *pConverterBase);
float converterbase_GetDelay(ConverterBase *pConverterBase);
void converterbase_Init(ConverterBase *pConverterBase, FilterSetup *flt_setup, int dsd_samples, int nDecimation);
int converterbase_Convert(ConverterBase *pConverterBase, uint8_t *lDsdData, double *pcm_data, int dsd_samples);

#endif
