/*
    Copyright 2015-2020 Robert Tari <robert@tari.in>
    Copyright 2011-2014 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>

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

#ifndef DECODER_H
#define DECODER_H

#include "decoderbase.h"
#include <pthread.h>

typedef enum
{
    DECODER_EMPTY,
    DECODER_LOADED,
    DECODER_RUNNING,
    DECODER_READY,
    DECODER_ERROR,
    DECODER_TERMINATING

} DecoderSlotState;

typedef struct
{
    volatile DecoderSlotState nDecoderSlotState;
    int nFrame;
    uint8_t *lDsdData;
    int nDsdSize;
    uint8_t *lDstData;
    int nDstSize;
    int nChannels;
    int nSampleRate;
    int nFrameRate;
    pthread_t hThread;
    pthread_cond_t hEventGet;
    pthread_cond_t hEventPut;
    pthread_mutex_t hMutex;
    DecoderBase cDecoderBase;

} DecoderFrameSlot;

typedef struct
{
    DecoderFrameSlot *lDecoderFrameSlots;
    int nThreads;
    int nChannels;
    int nSampleRate;
    int nFrameRate;
    uint32_t nFrame;
    int nSlot;

} Decoder;

Decoder* decoder_New(int nThreads);
void decoder_Free(Decoder *pDecoder);
int decoder_Init(Decoder *pDecoder, int nChannels, int nSampleRate, int nFrameRate);
int decoder_Decode(Decoder *pDecoder, uint8_t *lDstData, size_t nDstSize, uint8_t **pDsdData, size_t *pDsdSize);

#endif
