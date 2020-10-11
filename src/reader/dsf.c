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

#include "dsf.h"
#include <stdlib.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

#pragma pack(1)

typedef struct
{
    char sId[4];
    uint64_t nDataSize;

} Chunk;

typedef struct
{
    Chunk cChunk;
    uint32_t nVersion;
    uint32_t nFormat;
    uint32_t nChannelType;
    uint32_t nChannels;
    uint32_t nSampleRate;
    uint32_t nBitsPerSample;
    uint64_t nSamples;
    uint32_t nBlockSize;
    uint32_t nReserved;

} DsfChunk;

#pragma pack()

static bool dsf_IdEquals(const char *sId1, const char *sId2)
{
    return sId1[0] == sId2[0] && sId1[1] == sId2[1] && sId1[2] == sId2[2] && sId1[3] == sId2[3];
}

Dsf* dsf_New()
{
    Dsf *pDsf = malloc(sizeof(Dsf));

    for (int i = 0; i < 256; i++)
    {
        pDsf->lSwapBits[i] = 0;

        for (int j = 0; j < 8; j++)
        {
            pDsf->lSwapBits[i] |= ((i >> j) & 1) << (7 - j);
        }
    }

    pDsf->lBlockData = NULL;

    return pDsf;
}

void dsf_Free(Dsf *pDsf)
{
    if (pDsf->lBlockData)
    {
        free(pDsf->lBlockData);
    }

    free(pDsf);
}

uint32_t dsf_GetTrackCount(Dsf *pDsf, Area nArea)
{
    if ((nArea == AREA_TWOCH && pDsf->nChannels <= 2) || (nArea == AREA_MULCH && pDsf->nChannels > 2) || nArea == AREA_BOTH)
    {
        return 1;
    }

    return 0;
}

int dsf_GetChannels(Dsf *pDsf)
{
    return pDsf->nChannels;
}

int dsf_GetSampleRate(Dsf *pDsf)
{
    return pDsf->nSampleRate;
}

int dsf_GetFrameRate()
{
    return 75;
}

float dsf_GetProgress(Dsf *pDsf)
{
    return ((float)(media_GetPosition(pDsf->pMedia) - pDsf->nReadOffset) * 100.0) / (float)pDsf->nDataSize;
}

int dsf_Open(Dsf *pDsf, Media* pMedia)
{
    pDsf->pMedia = pMedia;
    Chunk ck;
    DsfChunk fmt;
    uint64_t pos;

    if (!(media_Read(pDsf->pMedia, &ck, sizeof(ck)) == sizeof(ck) && dsf_IdEquals(ck.sId, "DSD ")))
    {
        return false;
    }

    if (hton64(ck.nDataSize) != hton64((uint64_t)28))
    {
        return false;
    }

    if (media_Read(pDsf->pMedia, &pDsf->nFileSize, sizeof(pDsf->nFileSize)) != sizeof(pDsf->nFileSize))
    {
        return false;
    }

    media_Skip(pDsf->pMedia, sizeof(uint64_t));
    pos = media_GetPosition(pDsf->pMedia);

    if (!(media_Read(pDsf->pMedia, &fmt, sizeof(fmt)) == sizeof(fmt) && dsf_IdEquals(fmt.cChunk.sId, "fmt ")))
    {
        return false;
    }

    if (fmt.nFormat != 0)
    {
        return false;
    }

    if (fmt.nChannels < 1 || fmt.nChannels > 6)
    {
        return false;
    }

    pDsf->nChannels = fmt.nChannels;
    pDsf->nSampleRate = fmt.nSampleRate;

    switch (fmt.nBitsPerSample)
    {
        case 1:
            pDsf->bIsLsb = true;
            break;
        case 8:
            pDsf->bIsLsb = false;
            break;
        default:
            return false;
            break;
    }

    pDsf->nSamples = fmt.nSamples;
    pDsf->nBlockSize = fmt.nBlockSize;
    pDsf->nBlockOffset = pDsf->nBlockSize;
    pDsf->nBlockDataEnd = 0;
    //media_Seek(pDsf->pMedia, pos + hton64(fmt.cChunk.nDataSize), SEEK_SET);
    media_Seek(pDsf->pMedia, pos + fmt.cChunk.nDataSize, SEEK_SET);

    if (!(media_Read(pDsf->pMedia, &ck, sizeof(ck)) == sizeof(ck) && dsf_IdEquals(ck.sId, "data")))
    {
        return false;
    }

    pDsf->lBlockData = malloc(pDsf->nChannels * pDsf->nBlockSize);
    pDsf->nDataOffset = media_GetPosition(pDsf->pMedia);
    pDsf->nDataEndOffset = pDsf->nDataOffset + ((pDsf->nSamples / 8) * pDsf->nChannels);
    pDsf->nDataSize = hton64(hton64(ck.nDataSize)) - sizeof(ck);
    pDsf->nReadOffset = pDsf->nDataOffset;

    return 1;
}

char* dsf_SetTrack(Dsf *pDsf)
{
    media_Seek(pDsf->pMedia, pDsf->nDataOffset, SEEK_SET);

    return media_GetFileName(pDsf->pMedia);
}

bool dsf_ReadFrame(Dsf *pDsf, uint8_t *lFrameData, size_t *nFrameSize, FrameType *nFrameType)
{
    int samples_read = 0;

    for (int i = 0; i < (int)*nFrameSize / pDsf->nChannels; i++)
    {
        if (pDsf->nBlockOffset * pDsf->nChannels >= pDsf->nBlockDataEnd)
        {
                pDsf->nBlockDataEnd = (int)MIN(pDsf->nDataEndOffset - media_GetPosition(pDsf->pMedia), (uint64_t)(pDsf->nChannels * pDsf->nBlockSize));

            if (pDsf->nBlockDataEnd > 0)
            {
                pDsf->nBlockDataEnd = media_Read(pDsf->pMedia, pDsf->lBlockData, pDsf->nBlockDataEnd);
            }

            if (pDsf->nBlockDataEnd > 0)
            {
                pDsf->nBlockOffset = 0;
            }
            else
            {
                break;
            }
        }

        for (int ch = 0; ch < pDsf->nChannels; ch++)
        {
            uint8_t b = pDsf->lBlockData[ch * pDsf->nBlockSize + pDsf->nBlockOffset];
            lFrameData[i * pDsf->nChannels + ch] = pDsf->bIsLsb ? pDsf->lSwapBits[b] : b;
        }

        pDsf->nBlockOffset++;
        samples_read++;
    }

    *nFrameSize = samples_read * pDsf->nChannels;
    *nFrameType = samples_read > 0 ? FRAME_DSD : FRAME_INVALID;

    return samples_read > 0;
}
