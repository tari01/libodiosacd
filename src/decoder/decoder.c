/*
    Copyright 2015-2020 Robert Tari <robert@tari.in>
    Copyright 2011-2015 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>

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

#include "decoder.h"
#include <stdlib.h>
#include <string.h>

void* decoder_OnDecode(void* threadarg)
{
    DecoderFrameSlot* pDecoderFrameSlot = (DecoderFrameSlot*)threadarg;

    while (1)
    {
        pthread_mutex_lock(&pDecoderFrameSlot->hMutex);

        while (pDecoderFrameSlot->nDecoderSlotState != DECODER_LOADED && pDecoderFrameSlot->nDecoderSlotState != DECODER_TERMINATING)
        {
            pthread_cond_wait(&pDecoderFrameSlot->hEventPut, &pDecoderFrameSlot->hMutex);
        }

        if (pDecoderFrameSlot->nDecoderSlotState == DECODER_TERMINATING)
        {
            pDecoderFrameSlot->lDsdData = NULL;
            pDecoderFrameSlot->nDstSize = 0;
            pthread_mutex_unlock(&pDecoderFrameSlot->hMutex);
            return 0;
        }

        pDecoderFrameSlot->nDecoderSlotState = DECODER_RUNNING;
        pthread_mutex_unlock(&pDecoderFrameSlot->hMutex);

        bool bError = false;
        int nReturn = decoderbase_Decode(&pDecoderFrameSlot->cDecoderBase, pDecoderFrameSlot->lDstData, pDecoderFrameSlot->nDstSize * 8, pDecoderFrameSlot->lDsdData);

        if (nReturn == -1)
        {
            bError = true;
            decoderbase_Init(&pDecoderFrameSlot->cDecoderBase, pDecoderFrameSlot->nChannels, pDecoderFrameSlot->nSampleRate / 44100);
        }

        pthread_mutex_lock(&pDecoderFrameSlot->hMutex);
        pDecoderFrameSlot->nDecoderSlotState = bError ? DECODER_ERROR : DECODER_READY;
        pthread_cond_signal(&pDecoderFrameSlot->hEventGet);
        pthread_mutex_unlock(&pDecoderFrameSlot->hMutex);
    }

    return 0;
}

Decoder* decoder_New(int nThreads)
{
    Decoder *pDecoder = malloc(sizeof(Decoder));
    pDecoder->nThreads = nThreads;
    pDecoder->lDecoderFrameSlots = malloc(nThreads * sizeof(DecoderFrameSlot));

    if (!pDecoder->lDecoderFrameSlots)
    {
        pDecoder->nThreads = 0;
    }

    pDecoder->nChannels = 0;
    pDecoder->nSampleRate = 0;
    pDecoder->nFrameRate = 0;
    pDecoder->nSlot = 0;

    return pDecoder;
}

void decoder_Free(Decoder *pDecoder)
{
    for (int i = 0; i < pDecoder->nThreads; i++)
    {
        pthread_mutex_lock(&pDecoder->lDecoderFrameSlots[i].hMutex);
        pDecoder->lDecoderFrameSlots[i].nDecoderSlotState = DECODER_TERMINATING;
        pthread_cond_signal(&pDecoder->lDecoderFrameSlots[i].hEventPut);
        pthread_mutex_unlock(&pDecoder->lDecoderFrameSlots[i].hMutex);
        pthread_join(pDecoder->lDecoderFrameSlots[i].hThread, NULL);
        pthread_cond_destroy(&pDecoder->lDecoderFrameSlots[i].hEventGet);
        pthread_cond_destroy(&pDecoder->lDecoderFrameSlots[i].hEventPut);
        pthread_mutex_destroy(&pDecoder->lDecoderFrameSlots[i].hMutex);
    }

    free(pDecoder->lDecoderFrameSlots);
    free(pDecoder);
}

int decoder_Init(Decoder *pDecoder, int nChannels, int nSampleRate, int nFrameRate)
{
    for (int i = 0; i < pDecoder->nThreads; i++)
    {
        if (decoderbase_Init(&pDecoder->lDecoderFrameSlots[i].cDecoderBase, nChannels, (nSampleRate / 44100) / (nFrameRate / 75)) == 0)
        {
            pDecoder->lDecoderFrameSlots[i].nDecoderSlotState = DECODER_EMPTY;
            pDecoder->lDecoderFrameSlots[i].lDsdData = NULL;
            pDecoder->lDecoderFrameSlots[i].lDstData = NULL;
            pDecoder->lDecoderFrameSlots[i].nFrame = 0;
            pDecoder->lDecoderFrameSlots[i].nChannels = nChannels;
            pDecoder->lDecoderFrameSlots[i].nSampleRate = nSampleRate;
            pDecoder->lDecoderFrameSlots[i].nFrameRate = nFrameRate;
            pDecoder->lDecoderFrameSlots[i].nDsdSize = (size_t)(nSampleRate / 8 / nFrameRate * nChannels);
            pthread_mutex_init(&pDecoder->lDecoderFrameSlots[i].hMutex, NULL);
            pthread_cond_init(&pDecoder->lDecoderFrameSlots[i].hEventGet, NULL);
            pthread_cond_init(&pDecoder->lDecoderFrameSlots[i].hEventPut, NULL);
        }
        else
        {
            return -1;
        }

        pthread_create(&pDecoder->lDecoderFrameSlots[i].hThread, NULL, decoder_OnDecode, &pDecoder->lDecoderFrameSlots[i]);
    }

    pDecoder->nChannels = nChannels;
    pDecoder->nSampleRate = nSampleRate;
    pDecoder->nFrameRate = nFrameRate;
    pDecoder->nFrame = 0;

    return 0;
}

int decoder_Decode(Decoder *pDecoder, uint8_t* lDstData, size_t nDstSize, uint8_t** pDsdData, size_t *pDsdSize)
{
    pDecoder->lDecoderFrameSlots[pDecoder->nSlot].lDsdData = *pDsdData;
    pDecoder->lDecoderFrameSlots[pDecoder->nSlot].lDstData = lDstData;
    pDecoder->lDecoderFrameSlots[pDecoder->nSlot].nDstSize = nDstSize;
    pDecoder->lDecoderFrameSlots[pDecoder->nSlot].nFrame = pDecoder->nFrame;

    if (nDstSize > 0)
    {
        pthread_mutex_lock(&pDecoder->lDecoderFrameSlots[pDecoder->nSlot].hMutex);
        pDecoder->lDecoderFrameSlots[pDecoder->nSlot].nDecoderSlotState = DECODER_LOADED;
        pthread_cond_signal(&pDecoder->lDecoderFrameSlots[pDecoder->nSlot].hEventPut);
        pthread_mutex_unlock(&pDecoder->lDecoderFrameSlots[pDecoder->nSlot].hMutex);
    }
    else
    {
        pDecoder->lDecoderFrameSlots[pDecoder->nSlot].nDecoderSlotState = DECODER_EMPTY;
    }

    pDecoder->nSlot = (pDecoder->nSlot + 1) % pDecoder->nThreads;

    if (pDecoder->lDecoderFrameSlots[pDecoder->nSlot].nDecoderSlotState != DECODER_EMPTY)
    {
        pthread_mutex_lock(&pDecoder->lDecoderFrameSlots[pDecoder->nSlot].hMutex);

        while (pDecoder->lDecoderFrameSlots[pDecoder->nSlot].nDecoderSlotState != DECODER_READY && pDecoder->lDecoderFrameSlots[pDecoder->nSlot].nDecoderSlotState != DECODER_ERROR)
        {
            pthread_cond_wait(&pDecoder->lDecoderFrameSlots[pDecoder->nSlot].hEventGet, &pDecoder->lDecoderFrameSlots[pDecoder->nSlot].hMutex);
        }

        pthread_mutex_unlock(&pDecoder->lDecoderFrameSlots[pDecoder->nSlot].hMutex);
    }

    switch (pDecoder->lDecoderFrameSlots[pDecoder->nSlot].nDecoderSlotState)
    {
        case DECODER_READY:
            *pDsdData = pDecoder->lDecoderFrameSlots[pDecoder->nSlot].lDsdData;
            *pDsdSize = (size_t)(pDecoder->nSampleRate / 8 / pDecoder->nFrameRate * pDecoder->nChannels);
            break;
        case DECODER_ERROR:
            printf("\nPANIC: Failed to decode frame - inserting silence\n");
            *pDsdData = pDecoder->lDecoderFrameSlots[pDecoder->nSlot].lDsdData;
            *pDsdSize = (size_t)(pDecoder->nSampleRate / 8 / pDecoder->nFrameRate * pDecoder->nChannels);
            memset(*pDsdData, 0x69, *pDsdSize);
            break;
        default:
            *pDsdData = NULL;
            *pDsdSize = 0;
            break;
    }

    pDecoder->nFrame++;

    return 0;
}
