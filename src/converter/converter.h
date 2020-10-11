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

#ifndef CONVERTER_H
#define CONVERTER_H

#include <pthread.h>
#include "stdbool.h"
#include "converterbase.h"

typedef enum
{
    CONVERTER_EMPTY,
    CONVERTER_LOADED,
    CONVERTER_RUNNING,
    CONVERTER_READY,
    CONVERTER_TERMINATING

} ConverterSlotState;

typedef struct
{
    uint8_t *lDsdData;
    int nDsdSamples;
    double *lPcmData;
    int nPcmSamples;
    ConverterBase *pConverterBase;
    pthread_t hThread;
    pthread_cond_t hEventGet;
    pthread_cond_t hEventPut;
    pthread_mutex_t hMutex;
    volatile ConverterSlotState nConverterSlotState;

} ConverterSlot;

typedef struct
{
    int nChannels;
    int nFrameRate;
    int nDsdSampleRate;
    int nPcmSampleRate;
    float fDelay;
    bool bConvCalled;
    FilterSetup cFilterSetup;
    ConverterSlot *lConverterSlots;
    uint8_t lSwapBits[256];

} Converter;

Converter* converter_New();
float converter_GetDelay(Converter *pConverter);
bool converter_IsConvertCalled(Converter *pConverter);
int converter_Init(Converter *pConverter, int nChannels, int nFrameRate, int nDsdSampleRate, int nPcmSampleRate);
void converter_Free(Converter *pConverter);
int converter_Convert(Converter *pConverter, uint8_t *lDsdData, int nDsdSamples, float *lPcmData);

#endif
