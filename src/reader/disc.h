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

#ifndef DISC_H
#define DISC_H

#include "media.h"
#include "sacd.h"

typedef struct
{
    char *sTrackTitle;
    char *sTrackPerformer;
    int nChannels;

} TrackDetails;

typedef struct
{
    char *sAlbumTitle;
    char *sAlbumArtist;
    char *sAlbumPublisher;
    char *sAlbumCopyright;
    int nTwoChTracks;
    int nMulChTracks;
    TrackDetails *lTwoChTrackDetails;
    TrackDetails *lMulChTrackDetails;

} DiscDetails;

typedef struct
{
    uint8_t lData[65536];
    int nSize;
    bool bStarted;
    int nChannels;
    int nDstEncoded;

} AudioFrame;

typedef struct
{
    Media *pMedia;
    Sacd cSacd;
    Area nArea;
    uint32_t nTrackStartLsn;
    uint32_t nTrackLengthLsn;
    uint32_t nTrackCurrentLsn;
    uint8_t nChannels;
    AudioSector cAudioSector;
    AudioFrame cAudioFrame;
    int nPacketInfo;
    uint8_t lSector[2064];
    uint32_t nSectorSize;
    int nBadReads;
    uint8_t *lBuffer;
    int nOffset;
    DiscDetails cDiscDetails;

} Disc;

bool disc_IsSacd(const char *sPath);
Disc* disc_New();
void disc_Free(Disc *pDisc);
SacdArea* disc_GetArea(Disc *pDisc, Area nArea);
uint32_t disc_GetTrackCount(Disc *pDisc, Area nArea);
int disc_GetChannels(Disc *pDisc);
int disc_GetSampleRate();
int disc_GetFrameRate();
float disc_GetProgress(Disc *pDisc);
int disc_Open(Disc *pDisc, Media *pMedia);
bool disc_Close(Disc *pDisc);
char* disc_SetTrack(Disc *pDisc, uint32_t nTrack, Area nArea);
bool disc_ReadFrame(Disc *pDisc, uint8_t *lFrameData, size_t *pFrameSize, FrameType *pFrameType);
DiscDetails* disc_GetDiscDetails(Disc *pDisc);

#endif
