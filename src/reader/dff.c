/*
    Copyright 2015-2023 Robert Tari <robert@tari.in>
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

#include "dff.h"
#include <stdlib.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#pragma pack(1)

typedef struct
{
    char sId[4];
    uint64_t nDataSize;

} Chunk;

typedef struct
{
    uint64_t nOffset;
    uint32_t nLength;

} FrameIndex;

typedef enum
{
    TRACKSTART,
    TRACKSTOP

} MarkType;

typedef struct
{
    uint16_t nHours;
    uint8_t nMinutes;
    uint8_t nSeconds;
    uint32_t nSamples;
    int32_t nOffset;
    uint16_t nMarkType;
    uint16_t nChannel;
    uint16_t nTrackFlags;
    uint32_t nCount;

} Marker;

#pragma pack()

static double dff_GetMarkerTime(Dff *pDff, const Marker *pMarker)
{
    return (double)pMarker->nHours * 60 * 60 + (double)pMarker->nMinutes * 60 + (double)pMarker->nSeconds + ((double)pMarker->nSamples + (double)pMarker->nOffset) / (double)pDff->nSampleRate;
}

static bool dff_IdEquals(const char *sId1, const char *sId2)
{
    return sId1[0] == sId2[0] && sId1[1] == sId2[1] && sId1[2] == sId2[2] && sId1[3] == sId2[3];
}

static uint64_t dff_GetDstForFrame(Dff *pDff, uint32_t nFrame)
{
    FrameIndex frame_index;
    nFrame = MIN(nFrame, (uint32_t)(pDff->nDstSize / sizeof(FrameIndex) - 1));
    media_Seek(pDff->pMedia, pDff->nDstOffset + nFrame * sizeof(FrameIndex), SEEK_SET);
    uint64_t cur_offset = media_GetPosition (pDff->pMedia);
    media_Read(pDff->pMedia, &frame_index, sizeof(FrameIndex));
    media_Seek(pDff->pMedia, cur_offset, SEEK_SET);

    return hton64(frame_index.nOffset) - sizeof(Chunk);
}

Dff* dff_New()
{
    Dff *pDff = malloc(sizeof(Dff));
    pDff->nCurrentSubsong = 0;
    pDff->nDstEncoded = 0;
    pDff->lSubsongs = NULL;

    return pDff;
}

void dff_Free(Dff *pDff)
{
    dff_Close(pDff);
    free(pDff);
}

uint32_t dff_GetTrackCount(Dff *pDff, Area nArea)
{
    if ((nArea == AREA_TWOCH && pDff->nChannels == 2) || (nArea == AREA_MULCH && pDff->nChannels > 2) || nArea == AREA_BOTH)
    {
        return pDff->nSubsongs;
    }

    return 0;
}

int dff_GetChannels(Dff *pDff)
{
    return pDff->nChannels;
}

int dff_GetSampleRate(Dff *pDff)
{
    return pDff->nSampleRate;
}

int dff_GetFrameRate(Dff *pDff)
{
    return pDff->nFrameRate;
}

float dff_GetProgress(Dff *pDff)
{
    return ((float)(media_GetPosition(pDff->pMedia) - pDff->nCurrentOffset) * 100.0) / (float)pDff->nCurrentSize;
}

int dff_Open(Dff *pDff, Media *pMedia)
{
    pDff->pMedia = pMedia;
    pDff->nDstSize = 0;
    Chunk ck;
    char sId[4];
    uint32_t start_mark_count = 0;
    //pDff->lSubsongs = NULL;
    pDff->nSubsongs = 0;

    if (!media_Seek(pDff->pMedia, 0, SEEK_SET))
    {
        return 0;
    }

    if (!(media_Read(pDff->pMedia, &ck, sizeof(ck)) == sizeof(ck) && dff_IdEquals(ck.sId, "FRM8")))
    {
        return 0;
    }

    if (!(media_Read(pDff->pMedia, &sId, sizeof(sId)) == sizeof(sId) && dff_IdEquals(sId, "DSD ")))
    {
        return 0;
    }

    uint64_t nSize = hton64(ck.nDataSize);

    while ((uint64_t)media_GetPosition(pDff->pMedia) < nSize + sizeof(ck))
    {
        if (!(media_Read(pDff->pMedia, &ck, sizeof(ck)) == sizeof(ck)))
        {
            return 0;
        }

        if (dff_IdEquals(ck.sId, "PROP"))
        {
            if (!(media_Read(pDff->pMedia, &sId, sizeof(sId)) == sizeof(sId) && dff_IdEquals(sId, "SND ")))
            {
                return 0;
            }

            int64_t id_prop_end = media_GetPosition(pDff->pMedia) - sizeof(sId) + hton64(ck.nDataSize);

            while (media_GetPosition(pDff->pMedia) < id_prop_end)
            {
                if (!(media_Read(pDff->pMedia, &ck, sizeof(ck)) == sizeof(ck)))
                {
                    return 0;
                }

                if (dff_IdEquals(ck.sId, "FS  ") && hton64(ck.nDataSize) == 4)
                {
                    uint32_t nSampleRate;

                    if (!(media_Read(pDff->pMedia, &nSampleRate, sizeof(nSampleRate)) == sizeof(nSampleRate)))
                    {
                        return 0;
                    }

                    pDff->nSampleRate = hton32(nSampleRate);
                }
                else if (dff_IdEquals(ck.sId, "CHNL"))
                {
                    uint16_t nChannels;

                    if (!(media_Read(pDff->pMedia, &nChannels, sizeof(nChannels)) == sizeof(nChannels)))
                    {
                        return false;
                    }

                    pDff->nChannels = hton16(nChannels);
                    media_Skip(pDff->pMedia, hton64(ck.nDataSize) - sizeof(pDff->nChannels));
                }
                else if (dff_IdEquals(ck.sId, "CMPR"))
                {
                    if (!(media_Read(pDff->pMedia, &sId, sizeof(sId)) == sizeof(sId)))
                    {
                        return 0;
                    }

                    if (dff_IdEquals(sId, "DSD "))
                    {
                        pDff->nDstEncoded = 0;
                    }

                    if (dff_IdEquals(sId, "DST "))
                    {
                        pDff->nDstEncoded = 1;
                    }

                    media_Skip(pDff->pMedia, hton64(ck.nDataSize) - sizeof(sId));
                }
                else
                {
                    media_Skip(pDff->pMedia, hton64(ck.nDataSize));
                }

                media_Skip(pDff->pMedia, media_GetPosition(pDff->pMedia) & 1);
            }
        }
        else if (dff_IdEquals(ck.sId, "DSD "))
        {
            pDff->nDataOffset = media_GetPosition(pDff->pMedia);
            pDff->nDataSize = hton64(ck.nDataSize);
            pDff->nFrameRate = 75;
            pDff->nFrameSize = pDff->nSampleRate / 8 * pDff->nChannels / pDff->nFrameRate;
            pDff->nFrames = (uint32_t)(pDff->nDataSize / pDff->nFrameSize);
            media_Skip(pDff->pMedia, hton64(ck.nDataSize));
            pDff->lSubsongs = realloc(pDff->lSubsongs, ++pDff->nSubsongs * sizeof(Subsong));
            pDff->lSubsongs[pDff->nSubsongs - 1].fStartTime = 0.0;
            pDff->lSubsongs[pDff->nSubsongs - 1].fStopTime  = (double) pDff->nFrames / pDff->nFrameRate;
        }
        else if (dff_IdEquals(ck.sId, "DST "))
        {
            pDff->nDataOffset = media_GetPosition(pDff->pMedia);
            pDff->nDataSize = hton64(ck.nDataSize);

            if (!(media_Read(pDff->pMedia, &ck, sizeof(ck)) == sizeof(ck) && dff_IdEquals(ck.sId, "FRTE") && hton64(ck.nDataSize) == 6))
            {
                return 0;
            }

            pDff->nDataOffset += sizeof(ck) + hton64(ck.nDataSize);
            pDff->nDataSize -= sizeof(ck) + hton64(ck.nDataSize);
            pDff->nCurrentOffset = pDff->nDataOffset;
            pDff->nCurrentSize = pDff->nDataSize;
            uint32_t frame_count;

            if (!(media_Read(pDff->pMedia, &frame_count, sizeof(frame_count)) == sizeof(frame_count)))
            {
                return 0;
            }

            pDff->nFrames = hton32(frame_count);
            uint16_t nFrameRate;

            if (!(media_Read(pDff->pMedia, &nFrameRate, sizeof(nFrameRate)) == sizeof(nFrameRate)))
            {
                return 0;
            }

            pDff->nFrameRate = hton16(nFrameRate);
            pDff->nFrameSize = pDff->nSampleRate / 8 * pDff->nChannels / pDff->nFrameRate;
            media_Seek(pDff->pMedia, pDff->nDataOffset + pDff->nDataSize, SEEK_SET);
            pDff->lSubsongs = realloc(pDff->lSubsongs, ++pDff->nSubsongs * sizeof(Subsong));
            pDff->lSubsongs[pDff->nSubsongs - 1].fStartTime = 0.0;
            pDff->lSubsongs[pDff->nSubsongs - 1].fStopTime  = (double) pDff->nFrames / pDff->nFrameRate;
        }
        else if (dff_IdEquals(ck.sId, "DSTI"))
        {
            pDff->nDstOffset = media_GetPosition(pDff->pMedia);
            pDff->nDstSize = hton64(ck.nDataSize);
            media_Skip(pDff->pMedia, hton64(ck.nDataSize));
        }
        else if (dff_IdEquals(ck.sId, "DIIN"))
        {
            int64_t id_diin_end = media_GetPosition(pDff->pMedia) + hton64(ck.nDataSize);

            while (media_GetPosition(pDff->pMedia) < id_diin_end)
            {
                if (!(media_Read(pDff->pMedia, &ck, sizeof(ck)) == sizeof(ck)))
                {
                    return false;
                }

                if (dff_IdEquals(ck.sId, "MARK") && hton64(ck.nDataSize) >= sizeof(Marker))
                {
                    Marker cMarker;

                    if (media_Read(pDff->pMedia, &cMarker, sizeof(Marker)) == sizeof(Marker))
                    {
                        cMarker.nHours = hton16(cMarker.nHours);
                        cMarker.nSamples = hton32(cMarker.nSamples);
                        cMarker.nOffset = hton32(cMarker.nOffset);
                        cMarker.nMarkType = hton16(cMarker.nMarkType);
                        cMarker.nChannel = hton16(cMarker.nChannel);
                        cMarker.nTrackFlags = hton16(cMarker.nTrackFlags);
                        cMarker.nCount = hton32(cMarker.nCount);

                        switch (cMarker.nMarkType)
                        {
                            case TRACKSTART:
                            {
                                if (start_mark_count > 0)
                                {
                                    pDff->lSubsongs = realloc(pDff->lSubsongs, ++pDff->nSubsongs * sizeof(Subsong));
                                }

                                start_mark_count++;

                                if (pDff->nSubsongs > 0)
                                {
                                    pDff->lSubsongs[pDff->nSubsongs - 1].fStartTime = dff_GetMarkerTime(pDff, &cMarker);
                                    pDff->lSubsongs[pDff->nSubsongs - 1].fStopTime  = (double)pDff->nFrames / pDff->nFrameRate;

                                    if (pDff->nSubsongs - 1 > 0)
                                    {
                                        if (pDff->lSubsongs[pDff->nSubsongs - 2].fStopTime > pDff->lSubsongs[pDff->nSubsongs - 1].fStartTime)
                                        {
                                            pDff->lSubsongs[pDff->nSubsongs - 2].fStopTime =  pDff->lSubsongs[pDff->nSubsongs - 1].fStartTime;
                                        }
                                    }
                                }

                                break;
                            }
                            case TRACKSTOP:
                            {
                                if (pDff->nSubsongs > 0)
                                {
                                    pDff->lSubsongs[pDff->nSubsongs - 1].fStopTime = dff_GetMarkerTime(pDff, &cMarker);
                                }

                                break;
                            }
                        }
                    }

                    media_Skip(pDff->pMedia, hton64(ck.nDataSize) - sizeof(Marker));
                }
                else
                {
                    media_Skip(pDff->pMedia, hton64(ck.nDataSize));
                }

                media_Skip(pDff->pMedia, media_GetPosition(pDff->pMedia) & 1);
            }
        }
        else
        {
            media_Skip(pDff->pMedia, hton64(ck.nDataSize));
        }

        media_Skip(pDff->pMedia, media_GetPosition(pDff->pMedia) & 1);
    }

    media_Seek(pDff->pMedia, pDff->nDataOffset, SEEK_SET);

    return pDff->nSubsongs;
}

bool dff_Close(Dff *pDff)
{
    pDff->nCurrentSubsong = 0;

    if (pDff->lSubsongs)
    {
        free(pDff->lSubsongs);
    }

    pDff->nDstSize = 0;
    pDff->nSubsongs = 0;

    return true;
}

char* dff_SetTrack(Dff *pDff, uint32_t nTrack)
{
    if (nTrack < pDff->nSubsongs)
    {
        pDff->nCurrentSubsong = nTrack;
        double t0 = pDff->lSubsongs[pDff->nCurrentSubsong].fStartTime;
        double t1 = pDff->lSubsongs[pDff->nCurrentSubsong].fStopTime;
        uint64_t nOffset = (uint64_t)(t0 * pDff->nFrameRate / pDff->nFrames * pDff->nDataSize);
        uint64_t nSize = (uint64_t)(t1 * pDff->nFrameRate / pDff->nFrames * pDff->nDataSize) - nOffset;

        if (pDff->nDstEncoded)
        {
            if (pDff->nDstSize > 0)
            {
                if ((uint32_t)(t0 * pDff->nFrameRate) < (uint32_t)(pDff->nDstSize / sizeof(FrameIndex) - 1))
                {
                    pDff->nCurrentOffset = dff_GetDstForFrame(pDff, (uint32_t)(t0 * pDff->nFrameRate));
                }
                else
                {
                    pDff->nCurrentOffset = pDff->nDataOffset + nOffset;
                }

                if ((uint32_t)(t1 * pDff->nFrameRate) < (uint32_t)(pDff->nDstSize / sizeof(FrameIndex) - 1))
                {
                    pDff->nCurrentSize = dff_GetDstForFrame(pDff, (uint32_t)(t1 * pDff->nFrameRate)) - pDff->nCurrentOffset;
                }
                else
                {
                    pDff->nCurrentSize = nSize;
                }
            }
            else
            {
                pDff->nCurrentOffset = pDff->nDataOffset + nOffset;
                pDff->nCurrentSize = nSize;
            }
        }
        else
        {
            pDff->nCurrentOffset = pDff->nDataOffset + (nOffset / pDff->nFrameSize) * pDff->nFrameSize;
            pDff->nCurrentSize = (nSize / pDff->nFrameSize) * pDff->nFrameSize;
        }
    }

    media_Seek(pDff->pMedia, pDff->nCurrentOffset, SEEK_SET);

    return media_GetFileName(pDff->pMedia);
}

bool dff_ReadFrame(Dff *pDff, uint8_t *lFrameData, size_t *pFrameSize, FrameType *pFrameType)
{
    if (pDff->nDstEncoded)
    {
        Chunk ck;

        while ((uint64_t)media_GetPosition(pDff->pMedia) < pDff->nCurrentOffset + pDff->nCurrentSize && media_Read(pDff->pMedia, &ck, sizeof(ck)) == sizeof(ck))
        {
            if (dff_IdEquals(ck.sId, "DSTF") && hton64(ck.nDataSize) <= (uint64_t)*pFrameSize)
            {
                if (media_Read(pDff->pMedia, lFrameData, (size_t)hton64(ck.nDataSize)) == hton64(ck.nDataSize))
                {
                    media_Skip(pDff->pMedia, hton64(ck.nDataSize) & 1);
                    *pFrameSize = (size_t)hton64(ck.nDataSize);
                    *pFrameType = FRAME_DST;

                    return true;
                }

                break;
            }
            else if (dff_IdEquals(ck.sId, "DSTC") && hton64(ck.nDataSize) == 4)
            {
                uint32_t crc;

                if (hton64(ck.nDataSize) == sizeof(crc))
                {
                    if (media_Read(pDff->pMedia, &crc, sizeof(crc)) != sizeof(crc))
                    {
                        break;
                    }
                }
                else
                {
                    media_Skip(pDff->pMedia, hton64(ck.nDataSize));
                    media_Skip(pDff->pMedia, hton64(ck.nDataSize) & 1);
                }
            }
            else
            {
                media_Seek(pDff->pMedia, 1 - (int)sizeof(ck), SEEK_CUR);
            }
        }
    }
    else
    {
        uint64_t nPosition = media_GetPosition(pDff->pMedia);
        *pFrameSize = (size_t)MIN((int64_t)pDff->nFrameSize, (int64_t)MAX(0, (int64_t)(pDff->nCurrentOffset + pDff->nCurrentSize) - (int64_t)nPosition));

        if (*pFrameSize > 0)
        {
            *pFrameSize = media_Read(pDff->pMedia, lFrameData, *pFrameSize);
            *pFrameSize -= *pFrameSize % pDff->nChannels;

            if (*pFrameSize > 0)
            {
                *pFrameType = FRAME_DSD;
                return true;
            }
        }
    }

    *pFrameType = FRAME_INVALID;

    return false;
}
