/*
    Copyright 2015-2020 Robert Tari <robert@tari.in>
    Copyright 2011-2019 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>

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

#ifndef DSF_H
#define DSF_H

#include "media.h"
#include "sacd.h"

typedef struct
{
    Media* pMedia;
    int nSampleRate;
    int nChannels;
    uint64_t nFileSize;
    uint8_t *lBlockData;
    int nBlockSize;
    int nBlockOffset;
    int nBlockDataEnd;
    uint64_t nSamples;
    uint64_t nDataOffset;
    uint64_t nDataSize;
    uint64_t nDataEndOffset;
    uint64_t nReadOffset;
    bool bIsLsb;
    uint8_t lSwapBits[256];

} Dsf;

Dsf* dsf_New();
void dsf_Free(Dsf *pDsf);
uint32_t dsf_GetTrackCount(Dsf *pDsf, Area nArea);
int dsf_GetChannels(Dsf *pDsf);
int dsf_GetSampleRate(Dsf *pDsf);
int dsf_GetFrameRate();
float dsf_GetProgress(Dsf *pDsf);
int dsf_Open(Dsf *pDsf, Media* pMedia);
char* dsf_SetTrack(Dsf *pDsf);
bool dsf_ReadFrame(Dsf *pDsf, uint8_t* lFrameData, size_t* nFrameSize, FrameType* nFrameType);

#endif
