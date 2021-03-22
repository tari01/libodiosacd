/*
    Copyright 2015-2020 Robert Tari <robert@tari.in>

    SACD Ripper - http://code.google.com/p/sacd-ripper/
    Copyright 2010-2011 by respective authors.

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

#ifndef SACD_H
#define SACD_H

#include <stdint.h>

#define hton16(x) ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))
#define hton32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#define hton64(x) ((((x) & 0xff00000000000000ULL) >> 56) | (((x) & 0x00ff000000000000ULL) >> 40) | (((x) & 0x0000ff0000000000ULL) >> 24) | (((x) & 0x000000ff00000000ULL) >> 8) | (((x) & 0x00000000ff000000ULL) << 8) | (((x) & 0x0000000000ff0000ULL) << 24) | (((x) & 0x000000000000ff00ULL) << 40) | (((x) & 0x00000000000000ffULL) << 56))
#define SWAP16(x) x = (hton16(x))
#define SWAP32(x) x = (hton32(x))

typedef enum
{
    DATA_TYPE_AUDIO = 2,
    DATA_TYPE_SUPPLEMENTARY = 3,
    DATA_TYPE_PADDING = 7

} PacketType;

typedef enum
{
    AREA_BOTH = 0,
    AREA_TWOCH = 1,
    AREA_MULCH = 2,
    AREA_AUTO = 3

} Area;

typedef enum
{
    FRAME_DSD = 0,
    FRAME_DST = 1,
    FRAME_INVALID = -1

} FrameType;

typedef enum
{
    TRACK_TYPE_TITLE = 0x01,
    TRACK_TYPE_PERFORMER = 0x02,
    TRACK_TYPE_SONGWRITER = 0x03,
    TRACK_TYPE_COMPOSER = 0x04,
    TRACK_TYPE_ARRANGER = 0x05,
    TRACK_TYPE_MESSAGE = 0x06,
    TRACK_TYPE_EXTRA_MESSAGE = 0x07,
    TRACK_TYPE_TITLE_PHONETIC = 0x81,
    TRACK_TYPE_PERFORMER_PHONETIC = 0x82,
    TRACK_TYPE_SONGWRITER_PHONETIC = 0x83,
    TRACK_TYPE_COMPOSER_PHONETIC = 0x84,
    TRACK_TYPE_ARRANGER_PHONETIC = 0x85,
    TRACK_TYPE_MESSAGE_PHONETIC = 0x86,
    TRACK_TYPE_EXTRA_MESSAGE_PHONETIC = 0x87

} TrackType;

#pragma pack(1)

typedef struct
{
    uint8_t nCategory;
    uint16_t nReserved;
    uint8_t nGenre;

} Genre;

typedef struct
{
    char sLanguageCode[2];
    uint8_t nCharacterSet;
    uint8_t nReserved;

} Locale;

typedef struct
{
    char sId[8];

    struct
    {
        uint8_t nMajor;
        uint8_t nMinor;

    } cVersion;

    uint8_t lReserved1[6];
    uint16_t nAlbumSetSize;
    uint16_t nAlbumSequence;
    uint8_t lReserved2[4];
    char sAlbumCatalogueNumber[16];
    Genre lAlbumGenres[4];
    uint8_t lReserved3[8];
    uint32_t nArea1Toc1Start;
    uint32_t nArea1Toc2Start;
    uint32_t nArea2Toc1Start;
    uint32_t nArea2Toc2Start;
    uint8_t nDiscTypeReserved : 7;
    uint8_t nDiscTypeHybrid : 1;
    uint8_t lReserved4[3];
    uint16_t nArea1TocSize;
    uint16_t nArea2TocSize;
    char sDiscCatalogueNumber[16];
    Genre cGenreDisc[4];
    uint16_t nDiscDateYear;
    uint8_t nDiscDateMonth;
    uint8_t nDiscDateDay;
    uint8_t lReserved5[4];
    uint8_t nTextAreas;
    uint8_t lReserved6[7];
    Locale lLocales[8];

} MasterToc;

typedef struct
{
    char sId[8];
    uint8_t lReserved[8];
    uint16_t nAlbumTitlePosition;
    uint16_t nAlbumArtistPosition;
    uint16_t nAlbumPublisherPosition;
    uint16_t nAlbumCopyrightPosition;
    uint16_t nAlbumTitlePhoneticPosition;
    uint16_t nAlbumArtistPhoneticPosition;
    uint16_t nAlbumPublisherPhoneticPosition;
    uint16_t nAlbumCopyrightPhoneticPosition;
    uint16_t nDiscTitlePosition;
    uint16_t nDiscArtistPosition;
    uint16_t nDiscPublisherPosition;
    uint16_t nDiscCopyrightPosition;
    uint16_t nDiscTitlePhoneticPosition;
    uint16_t nDiscArtistPhoneticPosition;
    uint16_t nDiscPublisherPhoneticPosition;
    uint16_t nDiscCopyrightPhoneticPosition;
    uint8_t lData[2000];

} MasterTextPos;

typedef struct
{
    char *sAlbumTitle;
    char *sAlbumArtist;
    char *sAlbumPublisher;
    char *sAlbumCopyright;
    char *sAlbumTitlePhonetic;
    char *sAlbumArtistPhonetic;
    char *sAlbumPublisherPhonetic;
    char *sAlbumCopyrightPhonetic;
    char *sDiscTitle;
    char *sDiscArtist;
    char *sDiscPublisher;
    char *sDiscCopyright;
    char *sDiscTitlePhonetic;
    char *sDiscArtistPhonetic;
    char *sDiscPublisherPhonetic;
    char *sDiscCopyrightPhonetic;

} MasterText;

typedef struct
{
    char sId[8];
    uint8_t sInformation[2040];

} MasterMan;

typedef struct
{
    char sId[8];

    struct
    {
        uint8_t nMajor;
        uint8_t nMinor;

    } cVersion;

    uint16_t nSize;
    uint8_t lReserved01[4];
    uint32_t nMaxByteRate;
    uint8_t nSampleFrequency;
    uint8_t nFrameFormat : 4;
    uint8_t nReserved02 : 4;
    uint8_t lReserved03[10];
    uint8_t nChannels;
    uint8_t nSpeakerConfig : 5;
    uint8_t nExtraSettings : 3;
    uint8_t nMaxChannels;
    uint8_t nMuteFlags;
    uint8_t lReserved04[12];
    uint8_t nTrackAttribute : 4;
    uint8_t nReserved05 : 4;
    uint8_t lReserved06[15];

    struct
    {
        uint8_t nMinutes;
        uint8_t nSeconds;
        uint8_t nFrames;

    } PlayTime;

    uint8_t nReserved07;
    uint8_t nTrackOffset;
    uint8_t nTrackCount;
    uint8_t lReserved08[2];
    uint32_t nTrackStart;
    uint32_t nTrackEnd;
    uint8_t nTextAreas;
    uint8_t lReserved09[7];
    Locale lLocales[10];
    uint16_t nTrackTextOffset;
    uint16_t nIndexListOffset;
    uint16_t nAccessListOffset;
    uint8_t lReserved10[10];
    uint16_t nDescriptionOffset;
    uint16_t nCopyrightOffset;
    uint16_t nDescriptionPhoneticOffset;
    uint16_t nCopyrightPhoneticOffset;
    uint8_t lData[1896];

} AreaToc;

typedef struct
{
    char *sTrackTitle;
    char *sTrackPerformer;
    char *sTrackSongwriter;
    char *sTrackComposer;
    char *sTrackArranger;
    char *sTrackMessage;
    char *sTrackExtraMessage;
    char *sTrackTitlePhonetic;
    char *sTrackPerformerPhonetic;
    char *sTrackSongwriterPhonetic;
    char *sTrackComposerPhonetic;
    char *sTrackArrangerPhonetic;
    char *sTrackMessagePhonetic;
    char *sTrackExtraMessagePhonetic;

} AreaTrackText;

typedef struct
{
    char sId[8];
    uint16_t nTrackTextPosition[1];

} AreaText;

typedef struct
{
    char sCountryCode[2];
    char sOwnerCode[3];
    char sRecordingYear[2];
    char sDesignationCode[5];

} Isrc;

typedef struct
{
    char sId[8];
    Isrc cIsrc[255];
    uint32_t nReserved;
    Genre lTrackGenres[255];

} AreaIsrcGenre;

typedef struct
{
    char sId[8];
    uint32_t nTrackStart[255];
    uint32_t nLength[255];

} AreaTracklistOffset;

typedef struct
{
    uint8_t nMinutes;
    uint8_t nSeconds;
    uint8_t nFrames;
    uint8_t nReserved : 5;
    uint8_t nExtraUse : 3;

} AreaTracklistTimeStart;

typedef struct
{
    uint8_t nMinutes;
    uint8_t nSeconds;
    uint8_t nFrames;
    uint8_t nReserved : 3;
    uint8_t nTrackFlagsTmf1 : 1;
    uint8_t nTrackFlagsTmf2 : 1;
    uint8_t nTrackFlagsTmf3 : 1;
    uint8_t nTrackFlagsTmf4 : 1;
    uint8_t nTrackFlagsIlp : 1;

} AreaTracklistTimeDuration;

typedef struct
{
    char sId[8];
    AreaTracklistTimeStart lAreaTracklistTimeStart[255];
    AreaTracklistTimeDuration lAreaTracklistTimeDuration[255];

} AreaTracklistTime;

typedef struct
{
    uint8_t nFrameStart : 1;
    uint8_t nReserved : 1;
    uint8_t nDataType : 3;
    uint16_t nPacketLength : 11;

} AudioPacketInfo;

typedef struct
{
    struct
    {
        uint8_t nMinutes;
        uint8_t nSeconds;
        uint8_t nFrames;

    } TimeCode;

    uint8_t nChannelBit3 : 1;
    uint8_t nChannelBit2 : 1;
    uint8_t nSectorCount : 5;
    uint8_t nChannelBit1 : 1;

} FrameInfo;

typedef struct
{
    uint8_t nDstEncoded : 1;
    uint8_t nReserved : 1;
    uint8_t nFrameInfoCount : 3;
    uint8_t nPacketInfoCount : 3;

} AudioFrameHeader;

typedef struct
{
    AudioFrameHeader cAudioFrameHeader;
    AudioPacketInfo lAudioPacketInfos[7];
    FrameInfo lFrameInfos[7];

} AudioSector;

typedef struct
{
    uint8_t *pAreaData;
    AreaToc *pAreaToc;
    AreaTracklistOffset *pAreaTracklistOffset;
    AreaTracklistTime *pAreaTracklistTime;
    AreaText *pAreaText;
    AreaTrackText lAreaTrackTexts[255];
    AreaIsrcGenre *pAreaIsrcGenre;
    char *sDescription;
    char *sCopyright;
    char *sDescriptionPhonetic;
    char *sCopyrightPhonetic;
    char **lFileNames;

} SacdArea;

typedef struct
{
    void *pData;
    uint8_t *pMasterData;
    MasterToc *pMasterToc;
    MasterMan *pMasterMan;
    MasterText cMasterText;
    int nTwoChArea;
    int nMulChArea;
    int nAreas;
    SacdArea lSacdAreas[2];

} Sacd;

#pragma pack()

#endif
