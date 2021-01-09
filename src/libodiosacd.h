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

#ifndef LIBODIOSACD_H
#define LIBODIOSACD_H

#define LIBVERSION "21.1.9"

#include "reader/disc.h"
#include "stdbool.h"

typedef bool (*OnProgress)(float fProgress, char *sFilePath, int nTrack, void *pUserData);

bool odiolibsacd_Open(char *sInFile, Area nArea);
DiscDetails* odiolibsacd_GetDiscDetails();
int odiolibsacd_GetTrackCount(Area nArea);
bool odiolibsacd_Convert(char *sOutDir, int nSampleRate, OnProgress pOnProgress, void *pUserData);
void odiolibsacd_Close();

#endif
