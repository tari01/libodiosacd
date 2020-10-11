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

#include "disc.h"
#include <iconv.h>
#include <stdlib.h>
#include <string.h>

static const char *m_lCharacterSets[] =
{
    "US-ASCII",
    "ISO646-JP",
    "ISO-8859-1",
    "SHIFT_JISX0213",
    "KSC5636",
    "GB2312",
    "BIG5",
    "ISO-8859-1"
};

static char *string_Replace(const char *sHaystack, const char *sNeedle, const char *sReplace)
{
    int nPos = 0;
    int nFound = 0;
    int nReplaceLen = strlen(sReplace);
    int nNeedleLen = strlen(sNeedle);

    for (; sHaystack[nPos] != '\0'; nPos++)
    {
        if (strstr(&sHaystack[nPos], sNeedle) == &sHaystack[nPos])
        {
            nFound++;
            nPos += nNeedleLen - 1;
        }
    }

    char *sResult = malloc(nPos + nFound * (nReplaceLen - nNeedleLen) + 1);
    nPos = 0;

    while (*sHaystack)
    {
        if (strstr(sHaystack, sNeedle) == sHaystack)
        {
            strcpy(&sResult[nPos], sReplace);
            nPos += nReplaceLen;
            sHaystack += nNeedleLen;
        }
        else
        {
            sResult[nPos++] = *sHaystack++;
        }
    }

    sResult[nPos] = '\0';

    return sResult;
}

static char* string_ToUtf8(char *sTextOut, char *sTextIn, size_t nSizeIn, uint8_t nCodePage)
{
    if (nCodePage > 7)
    {
        nCodePage = 0;
    }

    if (sTextOut)
    {
        free(sTextOut);
    }

    size_t nSizeOut = nSizeIn * 2;
    sTextOut = calloc(nSizeOut, sizeof(char));
    char *pTextOut = sTextOut;

    iconv_t cd = iconv_open("UTF-8", m_lCharacterSets[nCodePage]);
    iconv(cd, &sTextIn, &nSizeIn, &pTextOut, &nSizeOut);
    iconv_close(cd);

    return sTextOut;
}

static bool disc_ReadBlocksRaw(Disc *pDisc, uint32_t nStart, size_t nBlocks, uint8_t *lData)
{
    switch (pDisc->nSectorSize)
    {
        case 2048:
        {
            media_Seek(pDisc->pMedia, (uint64_t)nStart * 2048, SEEK_SET);

            if (media_Read(pDisc->pMedia, lData, nBlocks * 2048) != nBlocks * 2048)
            {
                pDisc->nBadReads++;

                return false;
            }

            break;
        }
        case 2064:
        {
            for (uint32_t i = 0; i < nBlocks; i++)
            {
                media_Seek(pDisc->pMedia, (uint64_t)(nStart + i) * (uint64_t)2076, SEEK_SET);

                if (media_Read(pDisc->pMedia, lData + i * 2048, 2048) != 2048)
                {
                    pDisc->nBadReads++;

                    return false;
                }
            }

            break;
        }
    }

    return true;
}

static bool disc_ReadMasterToc(Disc *pDisc)
{
    uint8_t *p;
    MasterToc *pMasterToc;
    pDisc->cSacd.pMasterData = malloc(20480);

    if (!pDisc->cSacd.pMasterData)
    {
        return false;
    }

    if (!disc_ReadBlocksRaw(pDisc, 510, 10, pDisc->cSacd.pMasterData))
    {
        return false;
    }

    pMasterToc = pDisc->cSacd.pMasterToc = (MasterToc*)pDisc->cSacd.pMasterData;

    if (strncmp("SACDMTOC", pMasterToc->sId, 8) != 0)
    {
        return false;
    }

    SWAP32(pMasterToc->nArea1Toc1Start);
    SWAP32(pMasterToc->nArea1Toc2Start);
    SWAP16(pMasterToc->nArea1TocSize);
    SWAP32(pMasterToc->nArea2Toc1Start);
    SWAP32(pMasterToc->nArea2Toc2Start);
    SWAP16(pMasterToc->nArea2TocSize);
    SWAP16(pMasterToc->nDiscDateYear);

    if (pMasterToc->cVersion.nMajor > 1 || pMasterToc->cVersion.nMinor > 20)
    {
        return false;
    }

    p = pDisc->cSacd.pMasterData + 2048;

    for (int i = 0; i < 8; i++)
    {
        MasterTextPos *pMasterTextPos = (MasterTextPos*)p;

        if (strncmp("SACDText", pMasterTextPos->sId, 8) != 0)
        {
            return false;
        }

        SWAP16(pMasterTextPos->nAlbumTitlePosition);
        SWAP16(pMasterTextPos->nAlbumArtistPosition);
        SWAP16(pMasterTextPos->nAlbumPublisherPosition);
        SWAP16(pMasterTextPos->nAlbumCopyrightPosition);
        SWAP16(pMasterTextPos->nAlbumTitlePhoneticPosition);
        SWAP16(pMasterTextPos->nAlbumArtistPhoneticPosition);
        SWAP16(pMasterTextPos->nAlbumPublisherPhoneticPosition);
        SWAP16(pMasterTextPos->nAlbumCopyrightPhoneticPosition);
        SWAP16(pMasterTextPos->nDiscTitlePosition);
        SWAP16(pMasterTextPos->nDiscArtistPosition);
        SWAP16(pMasterTextPos->nDiscPublisherPosition);
        SWAP16(pMasterTextPos->nDiscCopyrightPosition);
        SWAP16(pMasterTextPos->nDiscTitlePhoneticPosition);
        SWAP16(pMasterTextPos->nDiscArtistPhoneticPosition);
        SWAP16(pMasterTextPos->nDiscPublisherPhoneticPosition);
        SWAP16(pMasterTextPos->nDiscCopyrightPhoneticPosition);

        if (i == 0)
        {
            uint8_t current_charset = pDisc->cSacd.pMasterToc->lLocales[i].nCharacterSet & 0x07;

            if (pMasterTextPos->nAlbumTitlePosition)
            {
                pDisc->cSacd.cMasterText.sAlbumTitle = string_ToUtf8(pDisc->cSacd.cMasterText.sAlbumTitle, (char*)pMasterTextPos + pMasterTextPos->nAlbumTitlePosition, strlen((char*)pMasterTextPos + pMasterTextPos->nAlbumTitlePosition), current_charset);
            }
            if (pMasterTextPos->nAlbumTitlePhoneticPosition)
                pDisc->cSacd.cMasterText.sAlbumTitlePhonetic = string_ToUtf8(pDisc->cSacd.cMasterText.sAlbumTitlePhonetic, (char*)pMasterTextPos + pMasterTextPos->nAlbumTitlePhoneticPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nAlbumTitlePhoneticPosition), current_charset);

            if (pMasterTextPos->nAlbumArtistPosition)
            {
                pDisc->cSacd.cMasterText.sAlbumArtist = string_ToUtf8(pDisc->cSacd.cMasterText.sAlbumArtist, (char*)pMasterTextPos + pMasterTextPos->nAlbumArtistPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nAlbumArtistPosition), current_charset);
            }

            if (pMasterTextPos->nAlbumArtistPhoneticPosition)
                pDisc->cSacd.cMasterText.sAlbumArtistPhonetic = string_ToUtf8(pDisc->cSacd.cMasterText.sAlbumArtistPhonetic, (char*)pMasterTextPos + pMasterTextPos->nAlbumArtistPhoneticPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nAlbumArtistPhoneticPosition), current_charset);

            if (pMasterTextPos->nAlbumPublisherPosition)
                pDisc->cSacd.cMasterText.sAlbumPublisher = string_ToUtf8(pDisc->cSacd.cMasterText.sAlbumPublisher, (char*)pMasterTextPos + pMasterTextPos->nAlbumPublisherPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nAlbumPublisherPosition), current_charset);

            if (pMasterTextPos->nAlbumPublisherPhoneticPosition)
                pDisc->cSacd.cMasterText.sAlbumPublisherPhonetic = string_ToUtf8(pDisc->cSacd.cMasterText.sAlbumPublisherPhonetic, (char*)pMasterTextPos + pMasterTextPos->nAlbumPublisherPhoneticPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nAlbumPublisherPhoneticPosition), current_charset);

            if (pMasterTextPos->nAlbumCopyrightPosition)
                pDisc->cSacd.cMasterText.sAlbumCopyright = string_ToUtf8(pDisc->cSacd.cMasterText.sAlbumCopyright, (char*)pMasterTextPos + pMasterTextPos->nAlbumCopyrightPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nAlbumCopyrightPosition), current_charset);

            if (pMasterTextPos->nAlbumCopyrightPhoneticPosition)
                pDisc->cSacd.cMasterText.sAlbumCopyrightPhonetic = string_ToUtf8(pDisc->cSacd.cMasterText.sAlbumCopyrightPhonetic, (char*)pMasterTextPos + pMasterTextPos->nAlbumCopyrightPhoneticPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nAlbumCopyrightPhoneticPosition), current_charset);

            if (pMasterTextPos->nDiscTitlePosition)
                pDisc->cSacd.cMasterText.sDiscTitle = string_ToUtf8(pDisc->cSacd.cMasterText.sDiscTitle, (char*)pMasterTextPos + pMasterTextPos->nDiscTitlePosition, strlen((char*)pMasterTextPos + pMasterTextPos->nDiscTitlePosition), current_charset);

            if (pMasterTextPos->nDiscTitlePhoneticPosition)
                pDisc->cSacd.cMasterText.sDiscTitlePhonetic = string_ToUtf8(pDisc->cSacd.cMasterText.sDiscTitlePhonetic, (char*)pMasterTextPos + pMasterTextPos->nDiscTitlePhoneticPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nDiscTitlePhoneticPosition), current_charset);

            if (pMasterTextPos->nDiscArtistPosition)
                pDisc->cSacd.cMasterText.sDiscArtist = string_ToUtf8(pDisc->cSacd.cMasterText.sDiscArtist, (char*)pMasterTextPos + pMasterTextPos->nDiscArtistPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nDiscArtistPosition), current_charset);

            if (pMasterTextPos->nDiscArtistPhoneticPosition)
                pDisc->cSacd.cMasterText.sDiscArtistPhonetic = string_ToUtf8(pDisc->cSacd.cMasterText.sDiscArtistPhonetic, (char*)pMasterTextPos + pMasterTextPos->nDiscArtistPhoneticPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nDiscArtistPhoneticPosition), current_charset);

            if (pMasterTextPos->nDiscPublisherPosition)
                pDisc->cSacd.cMasterText.sDiscPublisher = string_ToUtf8(pDisc->cSacd.cMasterText.sDiscPublisher, (char*)pMasterTextPos + pMasterTextPos->nDiscPublisherPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nDiscPublisherPosition), current_charset);

            if (pMasterTextPos->nDiscPublisherPhoneticPosition)
                pDisc->cSacd.cMasterText.sDiscPublisherPhonetic = string_ToUtf8(pDisc->cSacd.cMasterText.sDiscPublisherPhonetic, (char*)pMasterTextPos + pMasterTextPos->nDiscPublisherPhoneticPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nDiscPublisherPhoneticPosition), current_charset);

            if (pMasterTextPos->nDiscCopyrightPosition)
                pDisc->cSacd.cMasterText.sDiscCopyright = string_ToUtf8(pDisc->cSacd.cMasterText.sDiscCopyright, (char*)pMasterTextPos + pMasterTextPos->nDiscCopyrightPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nDiscCopyrightPosition), current_charset);

            if (pMasterTextPos->nDiscCopyrightPhoneticPosition)
                pDisc->cSacd.cMasterText.sDiscCopyrightPhonetic = string_ToUtf8(pDisc->cSacd.cMasterText.sDiscCopyrightPhonetic, (char*)pMasterTextPos + pMasterTextPos->nDiscCopyrightPhoneticPosition, strlen((char*)pMasterTextPos + pMasterTextPos->nDiscCopyrightPhoneticPosition), current_charset);
        }

        p += 2048;
    }

    pDisc->cSacd.pMasterMan = (MasterMan*)p;

    if (strncmp("SACD_Man", pDisc->cSacd.pMasterMan->sId, 8) != 0)
    {
        return false;
    }

    pDisc->cDiscDetails.sAlbumTitle = pDisc->cSacd.cMasterText.sAlbumTitle;
    pDisc->cDiscDetails.sAlbumArtist = pDisc->cSacd.cMasterText.sAlbumArtist;
    pDisc->cDiscDetails.sAlbumPublisher = pDisc->cSacd.cMasterText.sAlbumPublisher;
    pDisc->cDiscDetails.sAlbumCopyright = pDisc->cSacd.cMasterText.sAlbumCopyright;

    return true;
}

static bool disc_ReadAreaToc(Disc *pDisc, int area_idx)
{
    AreaToc *pAreaToc;
    uint8_t *pAreaData;
    uint8_t *p;
    int sacd_text_idx = 0;
    SacdArea *pSacdArea = &pDisc->cSacd.lSacdAreas[area_idx];
    uint8_t current_charset;
    Area nArea;

    p = pAreaData = pSacdArea->pAreaData;
    pAreaToc = pSacdArea->pAreaToc = (AreaToc*)pAreaData;

    if (strncmp("TWOCHTOC", pAreaToc->sId, 8) == 0)
    {
        pDisc->cDiscDetails.lTwoChTrackDetails = malloc(pAreaToc->nTrackCount * sizeof(TrackDetails));
        pDisc->cDiscDetails.nTwoChTracks = pAreaToc->nTrackCount;
        nArea = AREA_TWOCH;
    }
    else if (strncmp("MULCHTOC", pAreaToc->sId, 8) == 0)
    {
        pDisc->cDiscDetails.lMulChTrackDetails = malloc(pAreaToc->nTrackCount * sizeof(TrackDetails));
        pDisc->cDiscDetails.nMulChTracks = pAreaToc->nTrackCount;
        nArea = AREA_MULCH;
    }
    else
    {
        return false;
    }

    SWAP16(pAreaToc->nSize);
    SWAP32(pAreaToc->nTrackStart);
    SWAP32(pAreaToc->nTrackEnd);
    SWAP16(pAreaToc->nDescriptionOffset);
    SWAP16(pAreaToc->nCopyrightOffset);
    SWAP16(pAreaToc->nDescriptionPhoneticOffset);
    SWAP16(pAreaToc->nCopyrightPhoneticOffset);
    SWAP32(pAreaToc->nMaxByteRate);
    SWAP16(pAreaToc->nTrackTextOffset);
    SWAP16(pAreaToc->nIndexListOffset);
    SWAP16(pAreaToc->nAccessListOffset);

    current_charset = pSacdArea->pAreaToc->lLocales[sacd_text_idx].nCharacterSet & 0x07;

    pSacdArea->sDescription = NULL;
    pSacdArea->sCopyright = NULL;
    pSacdArea->sDescriptionPhonetic = NULL;
    pSacdArea->sCopyrightPhonetic = NULL;

    if (pAreaToc->nCopyrightOffset)
        pSacdArea->sCopyright = string_ToUtf8(pSacdArea->sCopyright, (char*)pAreaToc + pAreaToc->nCopyrightOffset, strlen((char*)pAreaToc + pAreaToc->nCopyrightOffset), current_charset);

    if (pAreaToc->nCopyrightPhoneticOffset)
        pSacdArea->sCopyrightPhonetic = string_ToUtf8(pSacdArea->sCopyrightPhonetic, (char*)pAreaToc + pAreaToc->nCopyrightPhoneticOffset, strlen((char*)pAreaToc + pAreaToc->nCopyrightPhoneticOffset), current_charset);

    if (pAreaToc->nDescriptionOffset)
        pSacdArea->sDescription = string_ToUtf8(pSacdArea->sDescription, (char*)pAreaToc + pAreaToc->nDescriptionOffset, strlen((char*)pAreaToc + pAreaToc->nDescriptionOffset), current_charset);

    if (pAreaToc->nDescriptionPhoneticOffset)
        pSacdArea->sDescriptionPhonetic = string_ToUtf8(pSacdArea->sDescriptionPhonetic, (char*)pAreaToc + pAreaToc->nDescriptionPhoneticOffset, strlen((char*)pAreaToc + pAreaToc->nDescriptionPhoneticOffset), current_charset);

    if (pAreaToc->cVersion.nMajor > 1 || pAreaToc->cVersion.nMinor > 20)
    {
        return false;
    }

    if (pAreaToc->nChannels == 2 && pAreaToc->nSpeakerConfig == 0)
        pDisc->cSacd.nTwoChArea = area_idx;
    else
        pDisc->cSacd.nMulChArea = area_idx;

    p += 2048;

    while (p < (pAreaData + pAreaToc->nSize * 2048))
    {
        if (strncmp((char*)p, "SACDTTxt", 8) == 0)
        {
            if (sacd_text_idx == 0)
            {
                for (uint8_t i = 0; i < pAreaToc->nTrackCount; i++)
                {
                    AreaText *pAreaText;
                    uint8_t track_type, track_amount;
                    char *track_ptr;
                    pAreaText = pSacdArea->pAreaText = (AreaText*)p;
                    SWAP16(pAreaText->nTrackTextPosition[i]);

                    pSacdArea->lAreaTrackTexts[i].sTrackTitle = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackPerformer = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackSongwriter = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackComposer = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackArranger = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackMessage = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackExtraMessage = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackTitlePhonetic = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackPerformerPhonetic = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackSongwriterPhonetic = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackComposerPhonetic = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackArrangerPhonetic = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackMessagePhonetic = NULL;
                    pSacdArea->lAreaTrackTexts[i].sTrackExtraMessagePhonetic = NULL;

                    if (pAreaText->nTrackTextPosition[i] > 0)
                    {
                        track_ptr = (char*)(p + pAreaText->nTrackTextPosition[i]);
                        track_amount = *track_ptr;
                        track_ptr += 4;

                        for (uint8_t j = 0; j < track_amount; j++)
                        {
                            track_type = *track_ptr;
                            track_ptr++;
                            track_ptr++;

                            if (*track_ptr != 0)
                            {
                                switch (track_type)
                                {
                                    case TRACK_TYPE_TITLE:
                                        pSacdArea->lAreaTrackTexts[i].sTrackTitle = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackTitle, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_PERFORMER:
                                        pSacdArea->lAreaTrackTexts[i].sTrackPerformer = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackPerformer, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_SONGWRITER:
                                        pSacdArea->lAreaTrackTexts[i].sTrackSongwriter = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackSongwriter, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_COMPOSER:
                                        pSacdArea->lAreaTrackTexts[i].sTrackComposer = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackComposer, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_ARRANGER:
                                        pSacdArea->lAreaTrackTexts[i].sTrackArranger = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackArranger, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_MESSAGE:
                                        pSacdArea->lAreaTrackTexts[i].sTrackMessage = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackMessage, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_EXTRA_MESSAGE:
                                        pSacdArea->lAreaTrackTexts[i].sTrackExtraMessage = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackExtraMessage, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_TITLE_PHONETIC:
                                        pSacdArea->lAreaTrackTexts[i].sTrackTitlePhonetic = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackTitlePhonetic, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_PERFORMER_PHONETIC:
                                        pSacdArea->lAreaTrackTexts[i].sTrackPerformerPhonetic = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackPerformerPhonetic, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_SONGWRITER_PHONETIC:
                                        pSacdArea->lAreaTrackTexts[i].sTrackSongwriterPhonetic = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackSongwriterPhonetic, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_COMPOSER_PHONETIC:
                                        pSacdArea->lAreaTrackTexts[i].sTrackComposerPhonetic = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackComposerPhonetic, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_ARRANGER_PHONETIC:
                                        pSacdArea->lAreaTrackTexts[i].sTrackArrangerPhonetic = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackArrangerPhonetic, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_MESSAGE_PHONETIC:
                                        pSacdArea->lAreaTrackTexts[i].sTrackMessagePhonetic = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackMessagePhonetic, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                    case TRACK_TYPE_EXTRA_MESSAGE_PHONETIC:
                                        pSacdArea->lAreaTrackTexts[i].sTrackExtraMessagePhonetic = string_ToUtf8(pSacdArea->lAreaTrackTexts[i].sTrackExtraMessagePhonetic, track_ptr, strlen(track_ptr), current_charset);
                                        break;
                                }
                            }

                            if (j < track_amount - 1)
                            {
                                while (*track_ptr != 0)
                                    track_ptr++;

                                while (*track_ptr == 0)
                                    track_ptr++;
                            }
                        }
                    }

                    if (nArea == AREA_TWOCH)
                    {
                        pDisc->cDiscDetails.lTwoChTrackDetails[i].sTrackTitle = pSacdArea->lAreaTrackTexts[i].sTrackTitle;
                        pDisc->cDiscDetails.lTwoChTrackDetails[i].sTrackPerformer = pSacdArea->lAreaTrackTexts[i].sTrackPerformer;
                        pDisc->cDiscDetails.lTwoChTrackDetails[i].nChannels = pAreaToc->nChannels;
                    }
                    else
                    {
                        pDisc->cDiscDetails.lMulChTrackDetails[i].sTrackTitle = pSacdArea->lAreaTrackTexts[i].sTrackTitle;
                        pDisc->cDiscDetails.lMulChTrackDetails[i].sTrackPerformer = pSacdArea->lAreaTrackTexts[i].sTrackPerformer;
                        pDisc->cDiscDetails.lMulChTrackDetails[i].nChannels = pAreaToc->nChannels;
                    }
                }
            }

            sacd_text_idx++;
            p += 2048;
        }
        else if (strncmp((char*)p, "SACD_IGL", 8) == 0)
        {
            pSacdArea->pAreaIsrcGenre = (AreaIsrcGenre*)p;
            p += 4096;
        }
        else if (strncmp((char*)p, "SACD_ACC", 8) == 0)
        {
            p += 65536;
        }
        else if (strncmp((char*)p, "SACDTRL1", 8) == 0)
        {
            AreaTracklistOffset *tracklist;
            tracklist = pSacdArea->pAreaTracklistOffset = (AreaTracklistOffset*)p;

            for (uint8_t i = 0; i < pAreaToc->nTrackCount; i++)
            {
                SWAP32(tracklist->nTrackStart[i]);
                SWAP32(tracklist->nLength[i]);
            }

            p += 2048;
        }
        else if (strncmp((char*)p, "SACDTRL2", 8) == 0)
        {
            pSacdArea->pAreaTracklistTime = (AreaTracklistTime*)p;
            p += 2048;
        }
        else
        {
            break;
        }
    }

    return true;
}

static void disc_FreeArea(SacdArea *pSacdArea)
{
    for (uint8_t i = 0; i < pSacdArea->pAreaToc->nTrackCount; i++)
    {
        if (pSacdArea->lAreaTrackTexts[i].sTrackTitle)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackTitle);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackPerformer)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackPerformer);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackSongwriter)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackSongwriter);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackComposer)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackComposer);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackArranger)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackArranger);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackMessage)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackMessage);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackExtraMessage)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackExtraMessage);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackTitlePhonetic)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackTitlePhonetic);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackPerformerPhonetic)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackPerformerPhonetic);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackSongwriterPhonetic)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackSongwriterPhonetic);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackComposerPhonetic)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackComposerPhonetic);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackArrangerPhonetic)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackArrangerPhonetic);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackMessagePhonetic)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackMessagePhonetic);
        }

        if (pSacdArea->lAreaTrackTexts[i].sTrackExtraMessagePhonetic)
        {
            free(pSacdArea->lAreaTrackTexts[i].sTrackExtraMessagePhonetic);
        }

        pSacdArea->lAreaTrackTexts[i].sTrackTitle = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackPerformer = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackSongwriter = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackComposer = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackArranger = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackMessage = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackExtraMessage = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackTitlePhonetic = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackPerformerPhonetic = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackSongwriterPhonetic = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackComposerPhonetic = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackArrangerPhonetic = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackMessagePhonetic = NULL;
        pSacdArea->lAreaTrackTexts[i].sTrackExtraMessagePhonetic = NULL;

        if (pSacdArea->lFileNames && pSacdArea->lFileNames[i])
        {
            free(pSacdArea->lFileNames[i]);
            pSacdArea->lFileNames[i] = NULL;
        }
    }

    if (pSacdArea->sDescription)
    {
        free(pSacdArea->sDescription);
    }

    if (pSacdArea->sCopyright)
    {
        free(pSacdArea->sCopyright);
    }

    if (pSacdArea->sDescriptionPhonetic)
    {
        free(pSacdArea->sDescriptionPhonetic);
    }

    if (pSacdArea->sCopyrightPhonetic)
    {
        free(pSacdArea->sCopyrightPhonetic);
    }

    pSacdArea->sDescription = NULL;
    pSacdArea->sCopyright = NULL;
    pSacdArea->sDescriptionPhonetic = NULL;
    pSacdArea->sCopyrightPhonetic = NULL;

    if (pSacdArea->lFileNames)
    {
        free(pSacdArea->lFileNames);
        pSacdArea->lFileNames = NULL;
    }
}

bool disc_IsSacd(const char *sPath)
{
    FILE *file = fopen(sPath, "r");
    char sacdmtoc[8];
    fseek(file, 1044480, SEEK_SET);

    if (fread(sacdmtoc, 1, 8, file) == 8)
    {
        if (memcmp(sacdmtoc, "SACDMTOC", 8) == 0)
        {
            return true;
        }
    }

    fseek(file, 1052652, SEEK_SET);

    if (fread(sacdmtoc, 1, 8, file) == 8)
    {
        if (memcmp(sacdmtoc, "SACDMTOC", 8) == 0)
        {
            return true;
        }
    }

    return false;
}

Disc* disc_New()
{
    Disc *pDisc = malloc(sizeof(Disc));
    pDisc->cAudioSector.cAudioFrameHeader.nDstEncoded = 0;
    pDisc->nBadReads = 0;

    MasterText *mt = &pDisc->cSacd.cMasterText;
    mt->sAlbumTitle = NULL;
    mt->sAlbumArtist = NULL;
    mt->sAlbumTitlePhonetic = NULL;
    mt->sAlbumArtistPhonetic = NULL;
    mt->sAlbumPublisher = NULL;
    mt->sAlbumPublisherPhonetic = NULL;
    mt->sAlbumCopyright = NULL;
    mt->sAlbumCopyrightPhonetic = NULL;
    mt->sDiscTitle = NULL;
    mt->sDiscTitlePhonetic = NULL;
    mt->sDiscArtist = NULL;
    mt->sDiscArtistPhonetic = NULL;
    mt->sDiscPublisher = NULL;
    mt->sDiscPublisherPhonetic = NULL;
    mt->sDiscCopyright = NULL;
    mt->sDiscCopyrightPhonetic = NULL;

    pDisc->cDiscDetails.sAlbumTitle = NULL;
    pDisc->cDiscDetails.sAlbumArtist = NULL;
    pDisc->cDiscDetails.sAlbumPublisher = NULL;
    pDisc->cDiscDetails.sAlbumCopyright = NULL;
    pDisc->cDiscDetails.nTwoChTracks = 0;
    pDisc->cDiscDetails.nMulChTracks = 0;
    pDisc->cDiscDetails.lTwoChTrackDetails = NULL;
    pDisc->cDiscDetails.lMulChTrackDetails = NULL;

    return pDisc;
}

SacdArea* disc_GetArea(Disc *pDisc, Area nArea)
{
    switch (nArea)
    {
        case AREA_TWOCH:
            if (pDisc->cSacd.nTwoChArea != -1)
                return &pDisc->cSacd.lSacdAreas[pDisc->cSacd.nTwoChArea];
            break;
        case AREA_MULCH:
            if (pDisc->cSacd.nMulChArea != -1)
                return &pDisc->cSacd.lSacdAreas[pDisc->cSacd.nMulChArea];
            break;
        default:
            break;
    }

    return 0;
}

uint32_t disc_GetTrackCount(Disc *pDisc, Area nArea)
{
    SacdArea *pSacdArea = disc_GetArea(pDisc, nArea);

    if (pSacdArea)
    {
        return pSacdArea->pAreaToc->nTrackCount;
    }

    return 0;
}

int disc_GetChannels(Disc *pDisc)
{
    return disc_GetArea(pDisc, pDisc->nArea) ? disc_GetArea(pDisc, pDisc->nArea)->pAreaToc->nChannels : 0;
}

int disc_GetSampleRate()
{
    return 2822400;
}

int disc_GetFrameRate()
{
    return 75;
}

float disc_GetProgress(Disc *pDisc)
{
    return ((float)(pDisc->nTrackCurrentLsn - pDisc->nTrackStartLsn) * 100.0) / (float)pDisc->nTrackLengthLsn;
}

bool disc_IsDst()
{
    return false;
}

int disc_Open(Disc *pDisc, Media *pMedia)
{
    pDisc->pMedia = pMedia;
    pDisc->cSacd.pMasterData = NULL;
    pDisc->cSacd.lSacdAreas[0].pAreaData = NULL;
    pDisc->cSacd.lSacdAreas[1].pAreaData = NULL;
    pDisc->cSacd.lSacdAreas[0].lFileNames = NULL;
    pDisc->cSacd.lSacdAreas[1].lFileNames = NULL;
    pDisc->cSacd.nAreas = 0;
    pDisc->cSacd.nTwoChArea = -1;
    pDisc->cSacd.nMulChArea = -1;
    char sacdmtoc[8];
    pDisc->nSectorSize = 0;
    pDisc->nBadReads = 0;
    media_Seek(pDisc->pMedia, 1044480, SEEK_SET);

    if (media_Read(pDisc->pMedia, sacdmtoc, 8) == 8)
    {
        if (memcmp(sacdmtoc, "SACDMTOC", 8) == 0)
        {
            pDisc->nSectorSize = 2048;
            pDisc->lBuffer = pDisc->lSector;
        }
    }

    if (!media_Seek(pDisc->pMedia, 1058760, SEEK_SET))
    {
        disc_Close(pDisc);

        return 0;
    }

    if (media_Read(pDisc->pMedia, sacdmtoc, 8) == 8)
    {
        if (memcmp(sacdmtoc, "SACDMTOC", 8) == 0)
        {
            pDisc->nSectorSize = 2064;
            pDisc->lBuffer = pDisc->lSector + 12;
        }
    }

    if (!media_Seek(pDisc->pMedia, 0, SEEK_SET))
    {
        disc_Close(pDisc);

        return 0;
    }

    if (pDisc->nSectorSize == 0)
    {
        disc_Close(pDisc);

        return 0;
    }

    if (!disc_ReadMasterToc(pDisc))
    {
        disc_Close(pDisc);

        return 0;
    }

    if (pDisc->cSacd.pMasterToc->nArea1Toc1Start)
    {
        pDisc->cSacd.lSacdAreas[pDisc->cSacd.nAreas].pAreaData = malloc(pDisc->cSacd.pMasterToc->nArea1TocSize * 2048);

        if (!pDisc->cSacd.lSacdAreas[pDisc->cSacd.nAreas].pAreaData)
        {
            disc_Close(pDisc);

            return 0;
        }

        if (!disc_ReadBlocksRaw(pDisc, pDisc->cSacd.pMasterToc->nArea1Toc1Start, pDisc->cSacd.pMasterToc->nArea1TocSize, pDisc->cSacd.lSacdAreas[pDisc->cSacd.nAreas].pAreaData))
        {
            pDisc->cSacd.pMasterToc->nArea1Toc1Start = 0;
        }
        else if (disc_ReadAreaToc(pDisc, pDisc->cSacd.nAreas))
        {
            pDisc->cSacd.nAreas++;
        }
    }

    if (pDisc->cSacd.pMasterToc->nArea2Toc1Start)
    {
        pDisc->cSacd.lSacdAreas[pDisc->cSacd.nAreas].pAreaData = malloc(pDisc->cSacd.pMasterToc->nArea2TocSize * 2048);

        if (!pDisc->cSacd.lSacdAreas[pDisc->cSacd.nAreas].pAreaData)
        {
            disc_Close(pDisc);

            return 0;
        }

        if (!disc_ReadBlocksRaw(pDisc, pDisc->cSacd.pMasterToc->nArea2Toc1Start, pDisc->cSacd.pMasterToc->nArea2TocSize, pDisc->cSacd.lSacdAreas[pDisc->cSacd.nAreas].pAreaData))
        {
            pDisc->cSacd.pMasterToc->nArea2Toc1Start = 0;
        }
        else if (disc_ReadAreaToc(pDisc, pDisc->cSacd.nAreas))
        {
            pDisc->cSacd.nAreas++;
        }
    }

    int nTracks = 0;

    for (int nArea = 0; nArea < pDisc->cSacd.nAreas; nArea++)
    {
        SacdArea *pSacdArea = &pDisc->cSacd.lSacdAreas[nArea];
        int nTrackCount = pSacdArea->pAreaToc->nTrackCount;
        pSacdArea->lFileNames = malloc(nTrackCount * sizeof(char*));

        if (nTrackCount != nTracks)
        {
            nTracks += nTrackCount;
        }

        for (int nTrack = 0; nTrack < nTrackCount; nTrack++)
        {
            pSacdArea->lFileNames[nTrack] = NULL;
        }
    }

    return nTracks;
}

void disc_Free(Disc *pDisc)
{
    disc_Close(pDisc);
    free(pDisc);
}

bool disc_Close(Disc *pDisc)
{
    if (pDisc->cSacd.nTwoChArea != -1)
    {
        disc_FreeArea(&pDisc->cSacd.lSacdAreas[pDisc->cSacd.nTwoChArea]);
        free(pDisc->cSacd.lSacdAreas[pDisc->cSacd.nTwoChArea].pAreaData);
        pDisc->cSacd.lSacdAreas[pDisc->cSacd.nTwoChArea].pAreaData = NULL;
        pDisc->cSacd.nTwoChArea = -1;
    }

    if (pDisc->cSacd.nMulChArea != -1)
    {
        disc_FreeArea(&pDisc->cSacd.lSacdAreas[pDisc->cSacd.nMulChArea]);
        free(pDisc->cSacd.lSacdAreas[pDisc->cSacd.nMulChArea].pAreaData);
        pDisc->cSacd.lSacdAreas[pDisc->cSacd.nMulChArea].pAreaData = NULL;
        pDisc->cSacd.nMulChArea = -1;
    }

    pDisc->cSacd.nAreas = 0;
    MasterText *mt = &pDisc->cSacd.cMasterText;

    if (mt->sAlbumTitle)
    {
        free(mt->sAlbumTitle);
    }

    if (mt->sAlbumArtist)
    {
        free(mt->sAlbumArtist);
    }

    if (mt->sAlbumTitlePhonetic)
    {
        free(mt->sAlbumTitlePhonetic);
    }

    if (mt->sAlbumArtistPhonetic)
    {
        free(mt->sAlbumArtistPhonetic);
    }

    if (mt->sAlbumPublisher)
    {
        free(mt->sAlbumPublisher);
    }

    if (mt->sAlbumPublisherPhonetic)
    {
        free(mt->sAlbumPublisherPhonetic);
    }

    if (mt->sAlbumCopyright)
    {
        free(mt->sAlbumCopyright);
    }

    if (mt->sAlbumCopyrightPhonetic)
    {
        free(mt->sAlbumCopyrightPhonetic);
    }

    if (mt->sDiscTitle)
    {
        free(mt->sDiscTitle);
    }

    if (mt->sDiscTitlePhonetic)
    {
        free(mt->sDiscTitlePhonetic);
    }

    if (mt->sDiscArtist)
    {
        free(mt->sDiscArtist);
    }

    if (mt->sDiscArtistPhonetic)
    {
        free(mt->sDiscArtistPhonetic);
    }

    if (mt->sDiscPublisher)
    {
        free(mt->sDiscPublisher);
    }

    if (mt->sDiscPublisherPhonetic)
    {
        free(mt->sDiscPublisherPhonetic);
    }

    if (mt->sDiscCopyright)
    {
        free(mt->sDiscCopyright);
    }

    if (mt->sDiscCopyrightPhonetic)
    {
        free(mt->sDiscCopyrightPhonetic);
    }

    mt->sAlbumTitle = NULL;
    mt->sAlbumArtist = NULL;
    mt->sAlbumTitlePhonetic = NULL;
    mt->sAlbumArtistPhonetic = NULL;
    mt->sAlbumPublisher = NULL;
    mt->sAlbumPublisherPhonetic = NULL;
    mt->sAlbumCopyright = NULL;
    mt->sAlbumCopyrightPhonetic = NULL;
    mt->sDiscTitle = NULL;
    mt->sDiscTitlePhonetic = NULL;
    mt->sDiscArtist = NULL;
    mt->sDiscArtistPhonetic = NULL;
    mt->sDiscPublisher = NULL;
    mt->sDiscPublisherPhonetic = NULL;
    mt->sDiscCopyright = NULL;
    mt->sDiscCopyrightPhonetic = NULL;

    if (pDisc->cSacd.pMasterData)
    {
        free(pDisc->cSacd.pMasterData);
        pDisc->cSacd.pMasterData = NULL;
    }

    if (pDisc->cDiscDetails.lTwoChTrackDetails)
    {
        free(pDisc->cDiscDetails.lTwoChTrackDetails);
        pDisc->cDiscDetails.lTwoChTrackDetails = NULL;
    }

    if (pDisc->cDiscDetails.lMulChTrackDetails)
    {
        free(pDisc->cDiscDetails.lMulChTrackDetails);
        pDisc->cDiscDetails.lMulChTrackDetails = NULL;
    }

    return true;
}

DiscDetails* disc_GetDiscDetails(Disc *pDisc)
{
    return &pDisc->cDiscDetails;
}

char* disc_SetTrack(Disc *pDisc, uint32_t nTrack, Area nArea)
{
    if (nTrack < disc_GetTrackCount(pDisc, nArea))
    {
        SacdArea *pSacdArea = disc_GetArea(pDisc, nArea);
        pDisc->nArea = nArea;

        if (nTrack > 0)
        {
            pDisc->nTrackStartLsn = pSacdArea->pAreaTracklistOffset->nTrackStart[nTrack];
        }
        else
        {
            pDisc->nTrackStartLsn = pSacdArea->pAreaToc->nTrackStart;
        }

        if (nTrack < disc_GetTrackCount(pDisc, nArea) - 1)
        {
            pDisc->nTrackLengthLsn = pSacdArea->pAreaTracklistOffset->nTrackStart[nTrack + 1] - pDisc->nTrackStartLsn + 1;
        }
        else
        {
            pDisc->nTrackLengthLsn = pSacdArea->pAreaToc->nTrackEnd - pDisc->nTrackStartLsn;
        }

        pDisc->nTrackCurrentLsn = pDisc->nTrackStartLsn;
        pDisc->nChannels = pSacdArea->pAreaToc->nChannels;
        memset(&pDisc->cAudioSector, 0, sizeof(pDisc->cAudioSector));
        memset(&pDisc->cAudioFrame, 0, sizeof(pDisc->cAudioFrame));
        pDisc->nPacketInfo = 0;
        media_Seek(pDisc->pMedia, (uint64_t)pDisc->nTrackCurrentLsn * (uint64_t)pDisc->nSectorSize, SEEK_SET);

        char *sFormatted;

        if (strlen(pSacdArea->lAreaTrackTexts[nTrack].sTrackPerformer))
        {
            sFormatted = calloc(17 + strlen(pSacdArea->lAreaTrackTexts[nTrack].sTrackPerformer) + strlen(pSacdArea->lAreaTrackTexts[nTrack].sTrackTitle) + 1, 1);
            sprintf(sFormatted, "(%ich) %.2i. %s - %s.wav", pDisc->nChannels, nTrack + 1, pSacdArea->lAreaTrackTexts[nTrack].sTrackPerformer, pSacdArea->lAreaTrackTexts[nTrack].sTrackTitle);
        }
        else
        {
            sFormatted = calloc(14 + strlen(pSacdArea->lAreaTrackTexts[nTrack].sTrackTitle) + 1, 1);
            sprintf(sFormatted, "(%ich) %.2i. %s.wav", pDisc->nChannels, nTrack + 1, pSacdArea->lAreaTrackTexts[nTrack].sTrackTitle);
        }

        char *sReplaced1 = string_Replace(sFormatted, "/", "-");
        free(sFormatted);
        char *sReplaced2 = string_Replace(sReplaced1, "\\", "-");
        free(sReplaced1);
        pSacdArea->lFileNames[nTrack] = strdup(sReplaced2);
        free(sReplaced2);

        return pSacdArea->lFileNames[nTrack];
    }

    pDisc->nArea = AREA_BOTH;

    return NULL;
}

bool disc_ReadFrame(Disc *pDisc, uint8_t *lFrameData, size_t *nFrameSize, FrameType *nFrameType)
{
    pDisc->nBadReads = 0;

    while (pDisc->nTrackCurrentLsn < pDisc->nTrackStartLsn + pDisc->nTrackLengthLsn)
    {
        if (pDisc->nBadReads > 0)
        {
            pDisc->nOffset = 0;
            pDisc->nPacketInfo = 0;
            memset(&pDisc->cAudioSector, 0, sizeof(pDisc->cAudioSector));
            memset(&pDisc->cAudioFrame, 0, sizeof(pDisc->cAudioFrame));
            *nFrameType = FRAME_INVALID;

            return true;
        }

        if (pDisc->nPacketInfo == pDisc->cAudioSector.cAudioFrameHeader.nPacketInfoCount)
        {

            pDisc->nOffset = 0;
            pDisc->nPacketInfo = 0;
            size_t read_bytes = media_Read(pDisc->pMedia, pDisc->lSector, pDisc->nSectorSize);
            pDisc->nTrackCurrentLsn++;

            if (read_bytes != pDisc->nSectorSize)
            {
                pDisc->nBadReads++;
                continue;
            }

            memcpy(&pDisc->cAudioSector.cAudioFrameHeader, pDisc->lBuffer + pDisc->nOffset, 1);
            pDisc->nOffset += 1;

            for (uint8_t i = 0; i < pDisc->cAudioSector.cAudioFrameHeader.nPacketInfoCount; i++)
            {
                pDisc->cAudioSector.lAudioPacketInfos[i].nFrameStart = ((pDisc->lBuffer + pDisc->nOffset)[0] >> 7) & 1;
                pDisc->cAudioSector.lAudioPacketInfos[i].nDataType = ((pDisc->lBuffer + pDisc->nOffset)[0] >> 3) & 7;
                pDisc->cAudioSector.lAudioPacketInfos[i].nPacketLength = ((pDisc->lBuffer + pDisc->nOffset)[0] & 7) << 8 | (pDisc->lBuffer + pDisc->nOffset)[1];
                pDisc->nOffset += 2;
            }

            if (pDisc->cAudioSector.cAudioFrameHeader.nDstEncoded)
            {
                memcpy(pDisc->cAudioSector.lFrameInfos, pDisc->lBuffer + pDisc->nOffset, 4 * pDisc->cAudioSector.cAudioFrameHeader.nFrameInfoCount);
                pDisc->nOffset += 4 * pDisc->cAudioSector.cAudioFrameHeader.nFrameInfoCount;
            }
            else
            {
                for (uint8_t i = 0; i < pDisc->cAudioSector.cAudioFrameHeader.nFrameInfoCount; i++)
                {
                    memcpy(&pDisc->cAudioSector.lFrameInfos[i], pDisc->lBuffer + pDisc->nOffset, 3);
                    pDisc->nOffset += 3;
                }
            }
        }

        while (pDisc->nPacketInfo < pDisc->cAudioSector.cAudioFrameHeader.nPacketInfoCount && pDisc->nBadReads == 0)
        {
            AudioPacketInfo *pAudioPacketInfo = &pDisc->cAudioSector.lAudioPacketInfos[pDisc->nPacketInfo];

            switch (pAudioPacketInfo->nDataType)
            {
                case DATA_TYPE_AUDIO:
                    if (pDisc->cAudioFrame.bStarted)
                    {
                        if (pAudioPacketInfo->nFrameStart)
                        {
                            if (pDisc->cAudioFrame.nSize <= (int)(*nFrameSize))
                            {
                                memcpy(lFrameData, pDisc->cAudioFrame.lData, pDisc->cAudioFrame.nSize);
                                *nFrameSize = pDisc->cAudioFrame.nSize;
                            }
                            else
                            {
                                pDisc->nBadReads++;
                                continue;
                            }

                            *nFrameType = pDisc->nBadReads > 0 ? FRAME_INVALID : pDisc->cAudioFrame.nDstEncoded ? FRAME_DST : FRAME_DSD;
                            pDisc->cAudioFrame.bStarted = false;

                            return true;
                        }
                    }
                    else
                    {
                        if (pAudioPacketInfo->nFrameStart)
                        {
                            pDisc->cAudioFrame.nSize = 0;
                            pDisc->cAudioFrame.nDstEncoded = pDisc->cAudioSector.cAudioFrameHeader.nDstEncoded;
                            pDisc->cAudioFrame.bStarted = true;
                        }
                    }

                    if (pDisc->cAudioFrame.bStarted)
                    {
                        if (pDisc->cAudioFrame.nSize + pAudioPacketInfo->nPacketLength <= (int)(*nFrameSize) && pDisc->nOffset + pAudioPacketInfo->nPacketLength <= 2048)
                        {
                            memcpy(pDisc->cAudioFrame.lData + pDisc->cAudioFrame.nSize, pDisc->lBuffer + pDisc->nOffset, pAudioPacketInfo->nPacketLength);
                            pDisc->cAudioFrame.nSize += pAudioPacketInfo->nPacketLength;
                        }
                        else
                        {
                            pDisc->nBadReads++;
                            continue;
                        }
                    }
                    break;
                case DATA_TYPE_SUPPLEMENTARY:
                case DATA_TYPE_PADDING:
                    break;
                default:
                    break;
            }

            pDisc->nOffset += pAudioPacketInfo->nPacketLength;
            pDisc->nPacketInfo++;
        }
    }

    if (pDisc->cAudioFrame.bStarted)
    {
        if (pDisc->cAudioFrame.nSize <= (int)(*nFrameSize))
        {
            memcpy(lFrameData, pDisc->cAudioFrame.lData, pDisc->cAudioFrame.nSize);
            *nFrameSize = pDisc->cAudioFrame.nSize;
        }
        else
        {
            pDisc->nBadReads++;
            pDisc->nOffset = 0;
            pDisc->nPacketInfo = 0;
            memset(&pDisc->cAudioSector, 0, sizeof(pDisc->cAudioSector));
            memset(&pDisc->cAudioFrame, 0, sizeof(pDisc->cAudioFrame));
        }

        pDisc->cAudioFrame.bStarted = false;
        *nFrameType = pDisc->nBadReads > 0 ? FRAME_INVALID : pDisc->cAudioFrame.nDstEncoded ? FRAME_DST : FRAME_DSD;
        return true;
    }

    *nFrameType = FRAME_INVALID;

    return false;
}
