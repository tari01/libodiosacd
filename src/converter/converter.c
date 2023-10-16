/*
    Copyright (c) 2015-2023 Robert Tari <robert@tari.in>
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

#include "converter.h"
#include "memory.h"

static void* converter_OnConvert(void *threadarg)
{
    ConverterSlot *slot = (ConverterSlot*)(threadarg);

    while (1)
    {
        pthread_mutex_lock(&slot->hMutex);

        while (slot->nConverterSlotState != CONVERTER_LOADED && slot->nConverterSlotState != CONVERTER_TERMINATING)
        {
            pthread_cond_wait(&slot->hEventPut, &slot->hMutex);
        }

        if (slot->nConverterSlotState == CONVERTER_TERMINATING)
        {
            slot->nPcmSamples = 0;
            pthread_mutex_unlock(&slot->hMutex);
            return 0;
        }

        slot->nConverterSlotState = CONVERTER_RUNNING;
        pthread_mutex_unlock(&slot->hMutex);

        slot->nPcmSamples = converterbase_Convert(slot->pConverterBase, slot->lDsdData, slot->lPcmData, slot->nDsdSamples);

        pthread_mutex_lock(&slot->hMutex);
        slot->nConverterSlotState = CONVERTER_READY;
        pthread_cond_signal(&slot->hEventGet);
        pthread_mutex_unlock(&slot->hMutex);
    }

    return 0;
}

Converter* converter_New()
{
    Converter* pConverter = malloc(sizeof(Converter));
    pConverter->nChannels = 0;
    pConverter->nFrameRate = 0;
    pConverter->nDsdSampleRate = 0;
    pConverter->nPcmSampleRate = 0;
    pConverter->fDelay = 0.0f;
    pConverter->lConverterSlots = NULL;
    pConverter->bConvCalled = false;
    filtersetup_New(&pConverter->cFilterSetup);

    for (int i = 0; i < 256; i++)
    {
        pConverter->lSwapBits[i] = 0;

        for (int j = 0; j < 8; j++)
        {
            pConverter->lSwapBits[i] |= ((i >> j) & 1) << (7 - j);
        }
    }

    return pConverter;
}

static void converter_FreeSlots(Converter *pConverter)
{
    for (int ch = 0; ch < pConverter->nChannels; ch++)
    {
        ConverterSlot *slot = &pConverter->lConverterSlots[ch];

        pthread_mutex_lock(&slot->hMutex);
        slot->nConverterSlotState = CONVERTER_TERMINATING;
        pthread_cond_signal(&slot->hEventPut);
        pthread_mutex_unlock(&slot->hMutex);

        pthread_join(slot->hThread, NULL);
        pthread_cond_destroy(&slot->hEventGet);
        pthread_cond_destroy(&slot->hEventPut);
        pthread_mutex_destroy(&slot->hMutex);

        converterbase_Free(slot->pConverterBase);
        slot->pConverterBase = NULL;
        memFree(slot->lDsdData);
        slot->lDsdData = NULL;
        slot->nDsdSamples = 0;
        memFree(slot->lPcmData);
        slot->lPcmData = NULL;
        slot->nPcmSamples = 0;
    }

    free(pConverter->lConverterSlots);
    pConverter->lConverterSlots = NULL;
}

static int converter_Close(Converter *pConverter)
{
    if (pConverter->lConverterSlots)
    {
        converter_FreeSlots(pConverter);
        pConverter->lConverterSlots = NULL;
    }

    return 0;
}

void converter_Free(Converter *pConverter)
{
    filtersetup_Free(&pConverter->cFilterSetup);
    converter_Close(pConverter);
    pConverter->fDelay = 0.0f;
    free(pConverter);
}

float converter_GetDelay(Converter *pConverter)
{
    return pConverter->fDelay;
}

bool converter_IsConvertCalled(Converter *pConverter)
{
    return pConverter->bConvCalled;
}

static ConverterSlot* converter_InitSlots(Converter *pConverter)
{
    pConverter->lConverterSlots = calloc (pConverter->nChannels, sizeof (ConverterSlot));

    int nDsdSamples = pConverter->nDsdSampleRate / 8 / pConverter->nFrameRate;
    int nPcmSamples = pConverter->nPcmSampleRate / pConverter->nFrameRate;
    int nDecimation = pConverter->nDsdSampleRate / pConverter->nPcmSampleRate;

    for (int ch = 0; ch < pConverter->nChannels; ch++)
    {
        ConverterSlot *slot = &pConverter->lConverterSlots[ch];
        slot->nConverterSlotState = CONVERTER_EMPTY;
        slot->lDsdData = (uint8_t*)memAlloc(nDsdSamples * sizeof(uint8_t));
        slot->nDsdSamples = nDsdSamples;
        slot->lPcmData = (double*)memAlloc(nPcmSamples * sizeof(double));
        slot->nPcmSamples = 0;
        slot->pConverterBase = converterbase_New();
        converterbase_Init(slot->pConverterBase, &pConverter->cFilterSetup, nDsdSamples, nDecimation);
        pthread_mutex_init(&slot->hMutex, NULL);
        pthread_cond_init(&slot->hEventGet, NULL);
        pthread_cond_init(&slot->hEventPut, NULL);
        pthread_create(&slot->hThread, NULL, converter_OnConvert, slot);
    }

    return pConverter->lConverterSlots;
}

int converter_Init(Converter *pConverter, int nChannels, int nFrameRate, int nDsdSampleRate, int nPcmSampleRate)
{
    converter_Close(pConverter);

    pConverter->nChannels = nChannels;
    pConverter->nFrameRate = nFrameRate;
    pConverter->nDsdSampleRate = nDsdSampleRate;
    pConverter->nPcmSampleRate = nPcmSampleRate;
    pConverter->lConverterSlots = converter_InitSlots(pConverter);
    pConverter->fDelay = converterbase_GetDelay(pConverter->lConverterSlots[0].pConverterBase);
    pConverter->bConvCalled = false;

    return 0;
}

static int converter_ConvertR(Converter *pConverter, float *lPcmData)
{
    int nPcmSamples = 0;

    for (int ch = 0; ch < pConverter->nChannels; ch++)
    {
        ConverterSlot *slot = &pConverter->lConverterSlots[ch];

        for (int sample = 0; sample < slot->nDsdSamples / 2; sample++)
        {
            uint8_t temp = slot->lDsdData[slot->nDsdSamples - 1 - sample];
            slot->lDsdData[slot->nDsdSamples - 1 - sample] = pConverter->lSwapBits[slot->lDsdData[sample]];
            slot->lDsdData[sample] = pConverter->lSwapBits[temp];
        }

        pthread_mutex_lock(&pConverter->lConverterSlots[ch].hMutex);
        pConverter->lConverterSlots[ch].nConverterSlotState = CONVERTER_LOADED;
        pthread_cond_signal(&pConverter->lConverterSlots[ch].hEventPut);
        pthread_mutex_unlock(&pConverter->lConverterSlots[ch].hMutex);
    }

    for (int ch = 0; ch < pConverter->nChannels; ch++)
    {
        ConverterSlot *slot = &pConverter->lConverterSlots[ch];

        pthread_mutex_lock(&slot->hMutex);

        while (slot->nConverterSlotState != CONVERTER_READY)
        {
            pthread_cond_wait(&slot->hEventGet, &slot->hMutex);
        }

        pthread_mutex_unlock(&slot->hMutex);

        for (int sample = 0; sample < slot->nPcmSamples; sample++)
        {
            lPcmData[sample * pConverter->nChannels + ch] = (float)slot->lPcmData[sample];
        }

        nPcmSamples += slot->nPcmSamples;
    }

    return nPcmSamples;
}

static int converter_ConvertC(Converter *pConverter, uint8_t *lDsdData, int nDsdSamples, float *lPcmData)
{
    int nPcmSamples = 0;

    for (int ch = 0; ch < pConverter->nChannels; ch++)
    {
        ConverterSlot *slot = &pConverter->lConverterSlots[ch];
        slot->nDsdSamples = nDsdSamples / pConverter->nChannels;

        for (int sample = 0; sample < slot->nDsdSamples; sample++)
        {
            slot->lDsdData[sample] = lDsdData[sample * pConverter->nChannels + ch];
        }

        pthread_mutex_lock(&slot->hMutex);
        slot->nConverterSlotState = CONVERTER_LOADED;
        pthread_cond_signal(&slot->hEventPut);
        pthread_mutex_unlock(&slot->hMutex);
    }

    for (int ch = 0; ch < pConverter->nChannels; ch++)
    {
        ConverterSlot *slot = &pConverter->lConverterSlots[ch];

        pthread_mutex_lock(&slot->hMutex);

        while (slot->nConverterSlotState != CONVERTER_READY)
        {
            pthread_cond_wait(&slot->hEventGet, &slot->hMutex);
        }

        pthread_mutex_unlock(&slot->hMutex);

        for (int sample = 0; sample < slot->nPcmSamples; sample++)
        {
            lPcmData[sample * pConverter->nChannels + ch] = (float)slot->lPcmData[sample];
        }

        nPcmSamples += slot->nPcmSamples;
    }

    return nPcmSamples;
}

static int converter_ConvertL(Converter *pConverter, uint8_t *lDsdData, int nDsdSamples)
{
    for (int ch = 0; ch < pConverter->nChannels; ch++)
    {
        ConverterSlot *slot = &pConverter->lConverterSlots[ch];

        slot->nDsdSamples = nDsdSamples / pConverter->nChannels;

        for (int sample = 0; sample < slot->nDsdSamples; sample++)
        {
            slot->lDsdData[sample] = pConverter->lSwapBits[lDsdData[(slot->nDsdSamples - 1 - sample) * pConverter->nChannels + ch]];
        }

        pthread_mutex_lock(&pConverter->lConverterSlots[ch].hMutex);
        pConverter->lConverterSlots[ch].nConverterSlotState = CONVERTER_LOADED;
        pthread_cond_signal(&pConverter->lConverterSlots[ch].hEventPut);
        pthread_mutex_unlock(&pConverter->lConverterSlots[ch].hMutex);
    }

    for (int ch = 0; ch < pConverter->nChannels; ch++)
    {
        ConverterSlot *slot = &pConverter->lConverterSlots[ch];

        pthread_mutex_lock(&slot->hMutex);

        while (slot->nConverterSlotState != CONVERTER_READY)
        {
            pthread_cond_wait(&slot->hEventGet, &slot->hMutex);
        }

        pthread_mutex_unlock(&slot->hMutex);
    }

    return 0;
}

int converter_Convert(Converter *pConverter, uint8_t *lDsdData, int nDsdSamples, float *lPcmData)
{
    int nPcmSamples = 0;

    if (!lDsdData)
    {
        if (pConverter->lConverterSlots)
        {
            nPcmSamples = converter_ConvertR(pConverter, lPcmData);
        }

        return nPcmSamples;
    }

    if (!pConverter->bConvCalled)
    {
        if (pConverter->lConverterSlots)
        {
            converter_ConvertL(pConverter, lDsdData, nDsdSamples);
        }

        pConverter->bConvCalled = true;
    }

    if (pConverter->lConverterSlots)
    {
        nPcmSamples = converter_ConvertC(pConverter, lDsdData, nDsdSamples, lPcmData);
    }

    return nPcmSamples;
}
