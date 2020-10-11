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

#ifndef DFF_H
#define DFF_H

#include "media.h"
#include "sacd.h"

typedef struct
{
    double fStartTime;
    double fStopTime;

} Subsong;

typedef struct
{
    Media* pMedia;
    uint32_t nSampleRate;
    uint16_t nChannels;
    int nDstEncoded;
    uint64_t nDstOffset;
    uint64_t nDstSize;
    uint64_t nDataOffset;
    uint64_t nDataSize;
    uint16_t nFrameRate;
    uint32_t nFrameSize;
    uint32_t nFrames;
    Subsong *lSubsongs;
    uint8_t nSubsongs;
    uint32_t nCurrentSubsong;
    uint64_t nCurrentOffset;
    uint64_t nCurrentSize;

} Dff;

Dff* dff_New();
void dff_Free(Dff *pDff);
uint32_t dff_GetTrackCount(Dff *pDff, Area nArea);
int dff_GetChannels(Dff *pDff);
int dff_GetSampleRate(Dff *pDff);
int dff_GetFrameRate(Dff *pDff);
float dff_GetProgress(Dff *pDff);
int dff_Open(Dff *pDff, Media *pMedia);
bool dff_Close(Dff *pDff);
char* dff_SetTrack(Dff *pDff, uint32_t nTrack);
bool dff_ReadFrame(Dff *pDff, uint8_t *lFrameData, size_t *nFrameSize, FrameType *nFrameType);

#endif
