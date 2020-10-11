/*
    MPEG-4 Audio RM Module
    Lossless coding of 1-bit oversampled audio - DST (Direct Stream Transfer)

    This software was originally developed by:

    * Aad Rijnberg
    Philips Digital Systems Laboratories Eindhoven
    <aad.rijnberg@philips.com>

    * Fons Bruekers
    Philips Research Laboratories Eindhoven
    <fons.bruekers@philips.com>

    * Eric Knapen
    Philips Digital Systems Laboratories Eindhoven
    <h.w.m.knapen@philips.com>

    And edited by:

    * Richard Theelen
    Philips Digital Systems Laboratories Eindhoven
    <r.h.m.theelen@philips.com>

    * Maxim V.Anisiutkin
    <maxim.anisiutkin@gmail.com>

    * Robert Tari
    <robert@tari.in>

    in the course of development of the MPEG-4 Audio standard ISO-14496-1, 2 and 3.
    This software module is an implementation of a part of one or more MPEG-4 Audio
    tools as specified by the MPEG-4 Audio standard. ISO/IEC gives users of the
    MPEG-4 Audio standards free licence to this software module or modifications
    thereof for use in hardware or software products claiming conformance to the
    MPEG-4 Audio standards. Those intending to use this software module in hardware
    or software products are advised that this use may infringe existing patents.
    The original developers of this software of this module and their company,
    the subsequent editors and their companies, and ISO/EIC have no liability for
    use of this software module or modifications thereof in an implementation.
    Copyright is not released for non MPEG-4 Audio conforming products. The
    original developer retains full right to use this code for his/her own purpose,
    assign or donate the code to a third party and to inhibit third party from
    using the code for non MPEG-4 Audio conforming products. This copyright notice
    must be included in all copies of derivative works.

    Copyright © 2004-2020.

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

#ifndef FRAMEREADER_H
#define FRAMEREADER_H

#include "codedtable.h"
#include "strdata.h"
#include "stdbool.h"

typedef struct
{
    int nResolution;
    int lLengths[6][8];
    int lSegments[6];
    int lTable[6][8];

} Segment;

typedef struct
{
    int nFrame;
    int nChannels;
    int nFilters;
    int nPtables;
    int nFSample44;
    int lPredOrder[12];
    int lPTableLengths[12];
    int16_t lICoefA[12][1 << 7];
    int nDstCoded;
    long nCalcBytes;
    long nCalcBits;
    int lHalfProbs[6];
    int lHalfBits[6];
    Segment cSegmentF;
    uint8_t lFilter4Bit[6][75264];
    Segment cSegmentP;
    uint8_t lPTable4Bit[6][75264];
    int nPSameSegAsF;
    int nPSameMapAsF;
    int nFSameSegAllCh;
    int nFSameMapAllCh;
    int nPSameSegAllCh;
    int nPSameMapAllCh;
    int nSegAndMapBits;
    int nMaxFilters;
    int nMaxPTables;
    long nMaxFrameLen;
    long nByteStreamLen;
    long nBitStreamLen;
    long nBitsPerCh;

} FrameHeader;

int framereader_Log2RoundUp(long x);
int framereader_RiceDecode(StrData *pStrData, int m);
void framereader_ReadDsdFrame(StrData *pStrData, long nMaxFrameLen, int nChannels, uint8_t* lDsdFrame);
void framereader_ReadTableSegmentData(StrData *pStrData, int nChannels, int FrameLen, int MaxNrOfSegs, int MinSegLen, Segment *S, int *SameSegAllCh);
void framereader_CopySegmentData(FrameHeader *pFrameHeader);
void framereader_ReadSegmentData(StrData *pStrData, FrameHeader *pFrameHeader);
void framereader_ReadTableMappingData(StrData *pStrData, int nChannels, int MaxNrOfTables, Segment *S, int *NrOfTables, int *SameMapAllCh);
void framereader_CopyMappingData(FrameHeader *pFrameHeader);
void framereader_ReadMappingData(StrData *pStrData, FrameHeader *pFrameHeader);
void framereader_ReadFilterCoefSets(StrData *pStrData, int nChannels, FrameHeader *pFrameHeader, CodedTableF *pCodedTableF);
void framereader_ReadProbabilityTables(StrData *pStrData, FrameHeader *pFrameHeader, CodedTableP *pCodedTableP, int lPOne[12][1 << 6]);
void framereader_ReadArithmeticCodedData(StrData *pStrData, int nADataLen, uint8_t *lADataByte);

#endif
