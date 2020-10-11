/*
    Copyright (c) 2015-2020 Robert Tari <robert@tari.in>
    Copyright (c) 2011-2015 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>

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

#ifndef DSDFILTER_H
#define DSDFILTER_H

#include <stdint.h>

typedef double CTable[256];

typedef struct
{
    CTable *pTables;
    int nOrder;
    int nLength;
    int nDecimation;
    uint8_t *lBuffer;
    int nIndex;

} DsdFilter;

void dsdfilter_New(DsdFilter* pDsdFilter);
void dsdfilter_Init(DsdFilter* pDsdFilter, CTable *pTables, int nLength, int nDecimation);
void dsdfilter_Free(DsdFilter* pDsdFilter);
float dsdfilter_GetDelay(DsdFilter* pDsdFilter);
int dsdfilter_Run(DsdFilter* pDsdFilter, uint8_t *lDsdData, double *lPcmData, int nDsdSamples);

#endif
