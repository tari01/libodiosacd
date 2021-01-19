/*
    Copyright 2015-2020 Robert Tari <robert@tari.in>

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

#include "libodiosacd.h"
#include "reader/disc.h"
#include "reader/dff.h"
#include "reader/dsf.h"
#include "converter/converter.h"
#include "decoder/decoder.h"
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef enum
{
    UNK_TYPE = 0,
    ISO_TYPE = 1,
    DSDIFF_TYPE = 2,
    DSF_TYPE = 3

} MediaType;

typedef struct
{
    int nTrack;
    Area nArea;
    int nTrackInfo;

} TrackInfo;

typedef union
{
    Dff *pDff;
    Dsf *pDsf;
    Disc *pDisc;

} Reader;

typedef struct
{
    Media *pMedia;
    Reader cReader;
    Decoder *pDecoder;
    Converter *pConverter;
    uint8_t *lDstBuf;
    uint8_t *lDsdBuf;
    float *lPcmBuf;
    int nDsdBufSize;
    int nDstBufSize;
    int nSampleRate;
    int nFrameRate;
    int nPcmSamples;
    int nPcmDelta;
    float fProgress;
    int nChannels;
    unsigned int nChannelMap;
    bool bTrackCompleted;
    bool bTrimmed;
    int nTwoch;
    int nMulch;

} OdioLibSacd;

int m_nCpus;
int m_nThreads;
TrackInfo *m_lTrackInfos;
int m_nTrackInfos;
pthread_mutex_t m_hMutex;
char *m_sOutPath;
char *m_sInPath;
int m_nSampleRate;
int m_nFinished;
MediaType m_nMediaType;
int m_nTracks;
OnProgress m_pOnProgress;
DiscDetails *m_pDiscDetails;
OdioLibSacd *m_pOdioLibSacd;
bool m_bSameTrackCounts;
bool m_bAbort;
void *m_pUserData;
float m_fProgress;

void odiolibsacd_DoClose(OdioLibSacd *pOdioLibSacd)
{
    if (m_nMediaType == ISO_TYPE && pOdioLibSacd->cReader.pDisc)
    {
        disc_Free(pOdioLibSacd->cReader.pDisc);
    }
    else if (m_nMediaType == DSDIFF_TYPE && pOdioLibSacd->cReader.pDff)
    {
        dff_Free(pOdioLibSacd->cReader.pDff);
    }
    else if (m_nMediaType == DSF_TYPE && pOdioLibSacd->cReader.pDsf)
    {
        dsf_Free(pOdioLibSacd->cReader.pDsf);
    }

    if (pOdioLibSacd->pMedia)
    {
        media_Free(pOdioLibSacd->pMedia);
    }

    if (pOdioLibSacd->pConverter)
    {
        converter_Free(pOdioLibSacd->pConverter);
    }

    if (pOdioLibSacd->pDecoder)
    {
        decoder_Free(pOdioLibSacd->pDecoder);
    }

    if (pOdioLibSacd->lDstBuf)
    {
        free(pOdioLibSacd->lDstBuf);
    }

    if (pOdioLibSacd->lDsdBuf)
    {
        free(pOdioLibSacd->lDsdBuf);
    }

    if (pOdioLibSacd->lPcmBuf)
    {
        free(pOdioLibSacd->lPcmBuf);
    }
}

void odiolibsacd_PackageInt(unsigned char *lBuf, int nOffset, int nValue, int nBytes)
{
    lBuf[nOffset + 0] = (unsigned char)(nValue & 0xff);
    lBuf[nOffset + 1] = (unsigned char)((nValue >> 8) & 0xff);

    if (nBytes == 4)
    {
        lBuf[nOffset + 2] = (unsigned char) ((nValue >> 0x10) & 0xff);
        lBuf[nOffset + 3] = (unsigned char) ((nValue >> 0x18) & 0xff);
    }
}

void odiolibsacd_DoConvert(OdioLibSacd *pOdioLibSacd, uint8_t *lDsdData, int nDsdSamples, float *lPcmData)
{
    if (pOdioLibSacd->pConverter)
    {
        converter_Convert(pOdioLibSacd->pConverter, lDsdData, nDsdSamples, lPcmData);
    }
}

void odiolibsacd_WriteData(OdioLibSacd *pOdioLibSacd, FILE *pFile, int nOffset, int nFrames)
{
    int nTrim = 0;

    if (!pOdioLibSacd->bTrimmed)
    {
        nTrim = 30;
        pOdioLibSacd->bTrimmed = true;
    }

    int nSamples = (nFrames - nTrim) * pOdioLibSacd->nChannels;
    int nBytesOut = nSamples * 3;
    char *pSrc = (char*)(pOdioLibSacd->lPcmBuf + (nOffset + nTrim) * pOdioLibSacd->nChannels);
    char *pDst = malloc(sizeof(char) * nBytesOut);
    int nOut = 0;

    for (int nSample = 0; nSample < nSamples; nSample++, pSrc += 4)
    {
        float fSample = *(float*)(pSrc);
        fSample = MIN(fSample, 1.0);
        fSample = MAX(fSample, -1.0);
        fSample *= 8388608.0;

        int32_t nVal = lrintf(fSample);
        nVal = MIN(nVal, 8388607);
        nVal = MAX(nVal, -8388608);

        pDst[nOut++] = nVal;
        pDst[nOut++] = nVal >> 8;
        pDst[nOut++] = nVal >> 16;
    }

    fwrite(pDst, 1, nBytesOut, pFile);
    free(pDst);

    if (m_nMediaType == ISO_TYPE)
    {
        pOdioLibSacd->fProgress = disc_GetProgress(pOdioLibSacd->cReader.pDisc);
    }
    else if (m_nMediaType == DSDIFF_TYPE)
    {
        pOdioLibSacd->fProgress = dff_GetProgress(pOdioLibSacd->cReader.pDff);
    }
    else if (m_nMediaType == DSF_TYPE)
    {
        pOdioLibSacd->fProgress = dsf_GetProgress(pOdioLibSacd->cReader.pDsf);
    }
}

int odiolibsacd_DoOpen(OdioLibSacd *pOdioLibSacd, char *sPath)
{
    pOdioLibSacd->pMedia = NULL;
    pOdioLibSacd->pConverter = NULL;
    pOdioLibSacd->pDecoder = NULL;
    pOdioLibSacd->fProgress = 0;
    pOdioLibSacd->nPcmSamples = 0;
    pOdioLibSacd->nPcmDelta = 0;
    pOdioLibSacd->lDstBuf = NULL;
    pOdioLibSacd->lDsdBuf = NULL;
    pOdioLibSacd->lPcmBuf = NULL;
    pOdioLibSacd->bTrimmed = false;

    char sExt[4];
    strncpy(sExt, sPath + (strlen(sPath) - 3), 3);
    sExt[3] = '\0';

    for (int i = 0; i < 3; ++i)
    {
        sExt[i] = tolower(sExt[i]);
    }

    m_nMediaType = UNK_TYPE;

    if (!strcmp(sExt, "iso"))
    {
        m_nMediaType = ISO_TYPE;
    }
    else if (!strcmp(sExt, "dff"))
    {
        m_nMediaType = DSDIFF_TYPE;
    }
    else if (!strcmp(sExt, "dsf"))
    {
        m_nMediaType = DSF_TYPE;
    }

    if (m_nMediaType == UNK_TYPE)
    {
        printf("PANIC: Unknown media format\n");
        return 0;
    }

    pOdioLibSacd->pMedia = media_New(sPath);

    if (!pOdioLibSacd->pMedia)
    {
        printf("PANIC: Failed to initialise media\n");

        return 0;
    }

    if (m_nMediaType == ISO_TYPE)
    {
        pOdioLibSacd->cReader.pDisc = disc_New();

        if (!pOdioLibSacd->cReader.pDisc)
        {
            printf("PANIC: Failed to initialise reader\n");

            return 0;
        }
    }
    else if (m_nMediaType == DSDIFF_TYPE)
    {
        pOdioLibSacd->cReader.pDff = dff_New();

        if (!pOdioLibSacd->cReader.pDff)
        {
            printf("PANIC: Failed to initialise reader\n");

            return 0;
        }
   }
   else if (m_nMediaType == DSF_TYPE)
   {
        pOdioLibSacd->cReader.pDsf = dsf_New();

        if (!pOdioLibSacd->cReader.pDsf)
        {
            printf("PANIC: Failed to initialise reader\n");

            return 0;
        }
    }

    int nTracks = 0;

    if (m_nMediaType == ISO_TYPE)
    {
        nTracks = disc_Open(pOdioLibSacd->cReader.pDisc, pOdioLibSacd->pMedia);
    }
    else if (m_nMediaType == DSDIFF_TYPE)
    {
        nTracks = dff_Open(pOdioLibSacd->cReader.pDff, pOdioLibSacd->pMedia);
    }
    else if (m_nMediaType == DSF_TYPE)
    {
        nTracks = dsf_Open(pOdioLibSacd->cReader.pDsf, pOdioLibSacd->pMedia);
    }

    if (nTracks == 0)
    {
        printf("PANIC: Failed to parse media\n");

        return 0;
    }

    return nTracks;
}

char* odiolibsacd_Init(OdioLibSacd *pOdioLibSacd, uint32_t nSubsong, int nSampleRate, Area nArea)
{
    if (pOdioLibSacd->pConverter)
    {
        converter_Free(pOdioLibSacd->pConverter);
        pOdioLibSacd->pConverter = NULL;
    }

    if (pOdioLibSacd->pDecoder)
    {
        decoder_Free(pOdioLibSacd->pDecoder);
        pOdioLibSacd->pDecoder = NULL;
    }

    char *strFileName = NULL;

    if (m_nMediaType == ISO_TYPE)
    {
        strFileName = disc_SetTrack(pOdioLibSacd->cReader.pDisc, nSubsong, nArea);
        pOdioLibSacd->nSampleRate = disc_GetSampleRate();
        pOdioLibSacd->nFrameRate = disc_GetFrameRate();
        pOdioLibSacd->nChannels = disc_GetChannels(pOdioLibSacd->cReader.pDisc);
    }
    else if (m_nMediaType == DSDIFF_TYPE)
    {
        strFileName = dff_SetTrack(pOdioLibSacd->cReader.pDff, nSubsong);
        pOdioLibSacd->nSampleRate = dff_GetSampleRate(pOdioLibSacd->cReader.pDff);
        pOdioLibSacd->nFrameRate = dff_GetFrameRate(pOdioLibSacd->cReader.pDff);
        pOdioLibSacd->nChannels = dff_GetChannels(pOdioLibSacd->cReader.pDff);
    }
    else if (m_nMediaType == DSF_TYPE)
    {
        strFileName = dsf_SetTrack(pOdioLibSacd->cReader.pDsf);
        pOdioLibSacd->nSampleRate = dsf_GetSampleRate(pOdioLibSacd->cReader.pDsf);
        pOdioLibSacd->nFrameRate = dsf_GetFrameRate();
        pOdioLibSacd->nChannels = dsf_GetChannels(pOdioLibSacd->cReader.pDsf);
    }

    pOdioLibSacd->nPcmSamples = m_nSampleRate / pOdioLibSacd->nFrameRate;

    switch (pOdioLibSacd->nChannels)
    {
        case 1:
            pOdioLibSacd->nChannelMap = 1<<2;
            break;
        case 2:
            pOdioLibSacd->nChannelMap = 1<<0 | 1<<1;
            break;
        case 3:
            pOdioLibSacd->nChannelMap = 1<<0 | 1<<1 | 1<<2;
            break;
        case 4:
            pOdioLibSacd->nChannelMap = 1<<0 | 1<<1 | 1<<4 | 1<<5;
            break;
        case 5:
            pOdioLibSacd->nChannelMap = 1<<0 | 1<<1 | 1<<2 | 1<<4 | 1<<5;
            break;
        case 6:
            pOdioLibSacd->nChannelMap = 1<<0 | 1<<1 | 1<<2 | 1<<3 | 1<<4 | 1<<5;
            break;
        default:
            pOdioLibSacd->nChannelMap = 0;
            break;
    }

    pOdioLibSacd->nDstBufSize = pOdioLibSacd->nDsdBufSize = pOdioLibSacd->nSampleRate / 8 / pOdioLibSacd->nFrameRate * pOdioLibSacd->nChannels;
    pOdioLibSacd->lDsdBuf = realloc(pOdioLibSacd->lDsdBuf, pOdioLibSacd->nDsdBufSize * m_nCpus * sizeof(uint8_t));
    pOdioLibSacd->lDstBuf = realloc(pOdioLibSacd->lDstBuf, pOdioLibSacd->nDstBufSize * m_nCpus * sizeof(uint8_t));
    pOdioLibSacd->lPcmBuf = realloc(pOdioLibSacd->lPcmBuf, pOdioLibSacd->nChannels * pOdioLibSacd->nPcmSamples * sizeof(float));
    pOdioLibSacd->pConverter = converter_New();
    converter_Init(pOdioLibSacd->pConverter, pOdioLibSacd->nChannels, pOdioLibSacd->nFrameRate, pOdioLibSacd->nSampleRate, m_nSampleRate);

    float fPcmOutDelay = converter_GetDelay(pOdioLibSacd->pConverter);
    pOdioLibSacd->nPcmDelta = (int)(fPcmOutDelay - 0.5f);//  + 0.5f originally

    if (pOdioLibSacd->nPcmDelta > pOdioLibSacd->nPcmSamples - 1)
    {
        pOdioLibSacd->nPcmDelta = pOdioLibSacd->nPcmSamples - 1;
    }

    pOdioLibSacd->bTrackCompleted = false;

    return strFileName;
}

void odiolibsacd_FixPcmStream(OdioLibSacd *pOdioLibSacd, bool bIsEnd, float *pPcmData, int nPcmSamples)
{
    if (!bIsEnd)
    {
        if (nPcmSamples > 1)
        {
            for (int ch = 0; ch < pOdioLibSacd->nChannels; ch++)
            {
                pPcmData[0 * pOdioLibSacd->nChannels + ch] = pPcmData[1 * pOdioLibSacd->nChannels + ch];
            }
        }
    }
    else
    {
        if (nPcmSamples > 1)
        {
            for (int ch = 0; ch < pOdioLibSacd->nChannels; ch++)
            {
                pPcmData[(nPcmSamples - 1) * pOdioLibSacd->nChannels + ch] = pPcmData[(nPcmSamples - 2) * pOdioLibSacd->nChannels + ch];
            }
        }
    }
}

bool odiolibsacd_Decode(OdioLibSacd *pOdioLibSacd, FILE *pFile)
{
    if (pOdioLibSacd->bTrackCompleted)
    {
        return true;
    }

    uint8_t *pDsdData;
    uint8_t *pDstData;
    size_t nDsdSize = 0;
    size_t nDstSize = 0;
    int nThread = 0;

    while (1)
    {
        nThread = pOdioLibSacd->pDecoder ? pOdioLibSacd->pDecoder->nSlot : 0;
        pDsdData = pOdioLibSacd->lDsdBuf + pOdioLibSacd->nDsdBufSize * nThread;
        pDstData = pOdioLibSacd->lDstBuf + pOdioLibSacd->nDstBufSize * nThread;
        nDstSize = pOdioLibSacd->nDstBufSize;
        FrameType nFrameType;
        bool bResult = false;

        if (m_nMediaType == ISO_TYPE)
        {
            bResult = disc_ReadFrame(pOdioLibSacd->cReader.pDisc, pDstData, &nDstSize, &nFrameType);
        }
        else if (m_nMediaType == DSDIFF_TYPE)
        {
            bResult = dff_ReadFrame(pOdioLibSacd->cReader.pDff, pDstData, &nDstSize, &nFrameType);
        }
        else if (m_nMediaType == DSF_TYPE)
        {
            bResult = dsf_ReadFrame(pOdioLibSacd->cReader.pDsf, pDstData, &nDstSize, &nFrameType);
        }

        if (bResult)
        {
            if (nDstSize > 0)
            {
                if (nFrameType == FRAME_INVALID)
                {
                    nDstSize = pOdioLibSacd->nDstBufSize;
                    memset(pDstData, 0x69, nDstSize);
                }

                if (nFrameType == FRAME_DST)
                {
                    if (!pOdioLibSacd->pDecoder)
                    {
                        pOdioLibSacd->pDecoder = decoder_New(m_nCpus);

                        if (!pOdioLibSacd->pDecoder || decoder_Init(pOdioLibSacd->pDecoder, pOdioLibSacd->nChannels, pOdioLibSacd->nSampleRate, pOdioLibSacd->nFrameRate) != 0)
                        {
                            return true;
                        }
                    }

                    decoder_Decode(pOdioLibSacd->pDecoder, pDstData, nDstSize, &pDsdData, &nDsdSize);
                }
                else
                {
                    pDsdData = pDstData;
                    nDsdSize = nDstSize;
                }

                if (nDsdSize > 0)
                {
                    int nRemoveSamples = 0;

                    if (pOdioLibSacd->pConverter && !converter_IsConvertCalled(pOdioLibSacd->pConverter))
                    {
                        nRemoveSamples = pOdioLibSacd->nPcmDelta;
                    }

                    odiolibsacd_DoConvert(pOdioLibSacd, pDsdData, nDsdSize, pOdioLibSacd->lPcmBuf);

                    if (nRemoveSamples > 0)
                    {
                        odiolibsacd_FixPcmStream(pOdioLibSacd, false, pOdioLibSacd->lPcmBuf + pOdioLibSacd->nChannels * nRemoveSamples, pOdioLibSacd->nPcmSamples - nRemoveSamples);
                    }

                    odiolibsacd_WriteData(pOdioLibSacd, pFile, nRemoveSamples, pOdioLibSacd->nPcmSamples - nRemoveSamples);

                    return false;
                }
            }
        }
        else
        {
            break;
        }
    }

    pDsdData = NULL;
    pDstData = NULL;
    nDstSize = 0;

    if (pOdioLibSacd->pDecoder)
    {
        decoder_Decode(pOdioLibSacd->pDecoder, pDstData, nDstSize, &pDsdData, &nDsdSize);
    }

    if (nDsdSize > 0)
    {
        odiolibsacd_DoConvert(pOdioLibSacd, pDsdData, nDsdSize, pOdioLibSacd->lPcmBuf);
        odiolibsacd_WriteData(pOdioLibSacd, pFile, 0, pOdioLibSacd->nPcmSamples);

        return false;
    }

    if (pOdioLibSacd->nPcmDelta > 0)
    {
        odiolibsacd_DoConvert(pOdioLibSacd, NULL, 0, pOdioLibSacd->lPcmBuf);
        odiolibsacd_FixPcmStream(pOdioLibSacd, true, pOdioLibSacd->lPcmBuf, pOdioLibSacd->nPcmDelta);
        odiolibsacd_WriteData(pOdioLibSacd, pFile, 0, pOdioLibSacd->nPcmDelta);
    }

    pOdioLibSacd->bTrackCompleted = true;

    return true;
}

void* odiolibsacd_OnProgress(void *pData)
{
    OdioLibSacd *lOdioLibSacd = (OdioLibSacd*)pData;

    while(1)
    {
        float fProgress = 0;

        for (int i = 0; i < m_nThreads; i++)
        {
            fProgress += lOdioLibSacd[i].fProgress;
        }

        m_fProgress = MAX(((((float)m_nTracks - (float)MIN(m_nThreads, m_nTracks) - (float)m_nTrackInfos) * 100.0) + fProgress) / (float)m_nTracks, 0);

        if (m_pOnProgress)
        {
            m_bAbort = !m_pOnProgress(m_fProgress, NULL, -1, m_pUserData);

            if (m_bAbort)
            {
                while (m_nFinished != m_nTracks)
                {
                    sleep(1);
                }

                return 0;
            }
        }

        if (m_nFinished == m_nTracks)
        {
            break;
        }

        sleep(1);
    }

    return 0;
}

void* odiolibsacd_OnDecode(void *pData)
{
    OdioLibSacd *pOdioLibSacd = (OdioLibSacd*)pData;

    while(m_nTrackInfos)
    {
        pthread_mutex_lock(&m_hMutex);

        TrackInfo cTrackInfo = m_lTrackInfos[0];

        for (int nTrackInfo = 1; nTrackInfo < m_nTrackInfos; nTrackInfo++)
        {
             m_lTrackInfos[nTrackInfo - 1] = m_lTrackInfos[nTrackInfo];
        }

        m_lTrackInfos = realloc(m_lTrackInfos, --m_nTrackInfos * sizeof(TrackInfo));

        pthread_mutex_unlock(&m_hMutex);

        if (m_bAbort)
        {
            m_nFinished++;

            continue;
        }

        char *sTrackName = odiolibsacd_Init(pOdioLibSacd, cTrackInfo.nTrack, m_nSampleRate, cTrackInfo.nArea);
        char *strOutFile = malloc(strlen(m_sOutPath) + strlen(sTrackName) + 1);
        strcpy(strOutFile, m_sOutPath);
        strcat(strOutFile, sTrackName);
        unsigned int nSize = 0x7fffffff;
        unsigned char arrHeader[68];
        unsigned char arrFormat[2] = {0xFE, 0xFF};
        unsigned char arrSubtype[16] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
        memcpy (arrHeader, "RIFF", 4);
        odiolibsacd_PackageInt (arrHeader, 4, nSize - 8, 4);
        memcpy (arrHeader + 8, "WAVE", 4);
        memcpy (arrHeader + 12, "fmt ", 4);
        odiolibsacd_PackageInt (arrHeader, 16, 40, 4);
        memcpy (arrHeader + 20, arrFormat, 2);
        odiolibsacd_PackageInt (arrHeader, 22, pOdioLibSacd->nChannels, 2);
        odiolibsacd_PackageInt (arrHeader, 24, m_nSampleRate, 4);
        odiolibsacd_PackageInt (arrHeader, 28, (m_nSampleRate * 24 * pOdioLibSacd->nChannels) / 8, 4);
        odiolibsacd_PackageInt (arrHeader, 32, pOdioLibSacd->nChannels * 3, 2);
        odiolibsacd_PackageInt (arrHeader, 34, 24, 2);
        odiolibsacd_PackageInt (arrHeader, 36, 22, 2);
        odiolibsacd_PackageInt (arrHeader, 38, 24, 2);
        odiolibsacd_PackageInt (arrHeader, 40, pOdioLibSacd->nChannelMap, 4);
        memcpy (arrHeader + 44, arrSubtype, 16);
        memcpy (arrHeader + 60, "data", 4);
        odiolibsacd_PackageInt (arrHeader, 64, nSize - 68, 4);
        FILE *pFile = fopen(strOutFile, "wb");
        fwrite(arrHeader, 1, 68, pFile);

        bool bDone = false;

        while (!bDone || !pOdioLibSacd->bTrackCompleted)
        {
            if (m_bAbort)
            {
                break;
            }
            else
            {
                bDone = odiolibsacd_Decode(pOdioLibSacd, pFile);
            }
        }

        if (!m_bAbort)
        {
            nSize = ftell(pFile);
            nSize -= 30 * pOdioLibSacd->nChannels * 3;
            odiolibsacd_PackageInt(arrHeader, 4, nSize - 8, 4);
            odiolibsacd_PackageInt(arrHeader, 64, nSize - 68, 4);
            fseek(pFile, 0, SEEK_SET);
            fwrite(arrHeader, 1, 68, pFile);
            fflush(pFile);
            int nFile = fileno(pFile);
            int nResult = ftruncate(nFile, nSize + 68);

            if (nResult == -1)
            {
                printf("PANIC: Could not trim file end\n");
            }
        }

        fclose(pFile);

        if (m_pOnProgress)
        {
            m_pOnProgress(m_fProgress, strOutFile, cTrackInfo.nTrackInfo, m_pUserData);
        }

        free(strOutFile);
        m_nFinished++;
    }

    return 0;
}

bool odiolibsacd_Open(char *sInFile, Area nArea)
{
    m_nCpus = 2;
    m_nThreads = 2;
    m_lTrackInfos = NULL;
    m_nTrackInfos = 0;
    m_sOutPath = NULL;
    m_sInPath = NULL;
    m_nSampleRate = 88200;
    m_nFinished = 0;
    m_nTracks = 0;
    m_pOnProgress = NULL;
    m_pDiscDetails = NULL;
    m_pOdioLibSacd = NULL;
    m_bSameTrackCounts = true;
    m_bAbort = false;
    m_pUserData = NULL;
    m_fProgress = 0.0;
    pthread_mutex_init(&m_hMutex, NULL);

    if (sInFile == NULL)
    {
        printf("PANIC: Invalid input file\n");

        return true;
    }

    m_sInPath = realpath(sInFile, NULL);
    struct stat cStat;

    if(stat(m_sInPath, &cStat) == -1 || !S_ISREG(cStat.st_mode))
    {
        printf("PANIC: \"%s\" is not a regular file\n", m_sInPath);
        free(m_sInPath);

        return true;
    }

    m_pOdioLibSacd = malloc(sizeof(OdioLibSacd));

    if (!odiolibsacd_DoOpen(m_pOdioLibSacd, m_sInPath))
    {
        free(m_sInPath);

        return true;
    }

    m_pOdioLibSacd->nTwoch = 0;
    m_pOdioLibSacd->nMulch = 0;

    if (m_nMediaType == ISO_TYPE)
    {
        m_pOdioLibSacd->nTwoch = disc_GetTrackCount(m_pOdioLibSacd->cReader.pDisc, AREA_TWOCH);
        m_pOdioLibSacd->nMulch = disc_GetTrackCount(m_pOdioLibSacd->cReader.pDisc, AREA_MULCH);
        m_pDiscDetails = disc_GetDiscDetails(m_pOdioLibSacd->cReader.pDisc);
    }
    else if (m_nMediaType == DSDIFF_TYPE)
    {
        m_pOdioLibSacd->nTwoch = dff_GetTrackCount(m_pOdioLibSacd->cReader.pDff, AREA_TWOCH);
        m_pOdioLibSacd->nMulch = dff_GetTrackCount(m_pOdioLibSacd->cReader.pDff, AREA_MULCH);
    }
    else if (m_nMediaType == DSF_TYPE)
    {
        m_pOdioLibSacd->nTwoch = dsf_GetTrackCount(m_pOdioLibSacd->cReader.pDsf, AREA_TWOCH);
        m_pOdioLibSacd->nMulch = dsf_GetTrackCount(m_pOdioLibSacd->cReader.pDsf, AREA_MULCH);
    }

    if (nArea == AREA_AUTO)
    {
        if (m_pOdioLibSacd->nMulch > 0 && m_pOdioLibSacd->nTwoch > 0 && m_pOdioLibSacd->nMulch != m_pOdioLibSacd->nTwoch)
        {
            nArea = AREA_BOTH;
            m_bSameTrackCounts = false;
        }
        else if (m_pOdioLibSacd->nMulch > 0)
        {
            nArea = AREA_MULCH;
        }
        else if (m_pOdioLibSacd->nTwoch > 0)
        {
            nArea = AREA_TWOCH;
        }
    }

    if (nArea == AREA_MULCH || nArea == AREA_BOTH)
    {
        if (nArea == AREA_MULCH && m_pOdioLibSacd->nMulch == 0)
        {
            printf("PANIC: The multichannel area has no tracks\n");
            free(m_sInPath);
            odiolibsacd_DoClose(m_pOdioLibSacd);
            free(m_pOdioLibSacd);

            return true;
        }

        for (int i = 0; i < m_pOdioLibSacd->nMulch; i++)
        {
            m_lTrackInfos = realloc(m_lTrackInfos, ++m_nTrackInfos * sizeof(TrackInfo));
            m_lTrackInfos[m_nTrackInfos - 1].nArea = AREA_MULCH;
            m_lTrackInfos[m_nTrackInfos - 1].nTrack = i;
            m_lTrackInfos[m_nTrackInfos - 1].nTrackInfo = m_nTrackInfos - 1;
            m_nTracks++;
        }
    }

    if (nArea == AREA_TWOCH || nArea == AREA_BOTH)
    {
        if (nArea == AREA_MULCH && m_pOdioLibSacd->nMulch == 0)
        {
            printf("PANIC: The stereo area has no tracks\n");
            free(m_sInPath);
            odiolibsacd_DoClose(m_pOdioLibSacd);
            free(m_pOdioLibSacd);

            return true;
        }

        for (int i = 0; i < m_pOdioLibSacd->nTwoch; i++)
        {
            m_lTrackInfos = realloc(m_lTrackInfos, ++m_nTrackInfos * sizeof(TrackInfo));
            m_lTrackInfos[m_nTrackInfos - 1].nArea = AREA_TWOCH;
            m_lTrackInfos[m_nTrackInfos - 1].nTrack = i;
            m_lTrackInfos[m_nTrackInfos - 1].nTrackInfo = m_nTrackInfos - 1;
            m_nTracks++;
        }
    }

    return false;
}

DiscDetails* odiolibsacd_GetDiscDetails()
{
    if (m_nMediaType != ISO_TYPE)
    {
        printf("PANIC: Media is not an SACD disc\n");

        return NULL;
    }

    return m_pDiscDetails;
}

int odiolibsacd_GetTrackCount(Area nArea)
{
    if (nArea == AREA_TWOCH)
    {
        return m_pOdioLibSacd->nTwoch;
    }
    else if (nArea == AREA_MULCH)
    {
        return m_pOdioLibSacd->nMulch;
    }
    else if (nArea == AREA_AUTO)
    {
        return m_nTracks;
    }

    printf("PANIC: Invalid area\n");

    return -1;
}

bool odiolibsacd_Convert(char *sOutDir, int nSampleRate, OnProgress pOnProgress, void *pUserData)
{
    if (m_pOdioLibSacd)
    {
        odiolibsacd_DoClose(m_pOdioLibSacd);
        free(m_pOdioLibSacd);
        m_pOdioLibSacd = NULL;
    }

    if (nSampleRate == 88200 || nSampleRate == 176400)
    {
        m_nSampleRate = nSampleRate;
    }
    else
    {
        printf("PANIC: Invalid samplerate\n");

        return true;
    }

    if (sOutDir == NULL)
    {
        char *pSlashPos = strrchr(m_sInPath, '/');
        m_sOutPath = strndup(m_sInPath, (int)(pSlashPos - m_sInPath) + 1);
    }
    else
    {
        m_sOutPath = strdup(sOutDir);
    }

    struct stat cStat;

    if (strlen(m_sOutPath) == 0 || stat(m_sOutPath, &cStat) == -1 || !S_ISDIR(cStat.st_mode))
    {
        printf("PANIC: Directory \"%s\" does not exist\n", m_sOutPath);
        free(m_sInPath);

        if (m_sOutPath != NULL)
        {
            free(m_sOutPath);
            m_sOutPath = NULL;
        }

        return true;
    }

    char *sOutPathTmp = strdup(m_sOutPath);
    free(m_sOutPath);
    m_sOutPath = realpath(sOutPathTmp, NULL);
    free(sOutPathTmp);

    if (m_sOutPath[strlen(m_sOutPath) - 1] != '/')
    {
        m_sOutPath = realloc(m_sOutPath, strlen(m_sOutPath) + 2);
        strcat(m_sOutPath, "/");
    }

    m_pOnProgress = pOnProgress;
    m_pUserData = pUserData;

    if (!m_bSameTrackCounts)
    {
        printf("WARNING: The multichannel and stereo areas have a different track count: extracting both.\n\n");
    }

    int nCpus = sysconf(_SC_NPROCESSORS_ONLN);

    if (nCpus > 2)
    {
        m_nCpus = nCpus;
    }

    m_nThreads = MIN(m_nCpus, m_nTrackInfos);
    pthread_t hThreadProgress;
    OdioLibSacd *lOdioLibSacd = malloc(m_nThreads * sizeof(OdioLibSacd));
    pthread_t *lThreads = malloc(m_nThreads * sizeof(pthread_t));

    for (int i = 0; i < m_nThreads; i++)
    {
        odiolibsacd_DoOpen(&lOdioLibSacd[i], m_sInPath);
        pthread_create(&lThreads[i], NULL, odiolibsacd_OnDecode, &lOdioLibSacd[i]);
        pthread_detach(lThreads[i]);
    }

    pthread_create(&hThreadProgress, NULL, odiolibsacd_OnProgress, lOdioLibSacd);
    pthread_join(hThreadProgress, NULL);
    pthread_mutex_destroy(&m_hMutex);

    for (int i = 0; i < m_nThreads; i++)
    {
        odiolibsacd_DoClose(&lOdioLibSacd[i]);
    }

    free(lOdioLibSacd);
    free(lThreads);

    if (m_lTrackInfos)
    {
        free(m_lTrackInfos);
    }

    return false;
}

void odiolibsacd_Close()
{
    if (m_pOdioLibSacd)
    {
        odiolibsacd_DoClose(m_pOdioLibSacd);
        free(m_pOdioLibSacd);
        m_pOdioLibSacd = NULL;
    }

    free(m_sInPath);
    free(m_sOutPath);
}
