/*
    Copyright 2015-2020 Robert Tari <robert@tari.in>
    Copyright 2011-2012 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>

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

#ifndef MEDIA_H
#define MEDIA_H

#include <stdint.h>
#include <stdio.h>
#include "stdbool.h"

typedef struct
{
    FILE *pFile;
    char *sFilePath;
    char *sFileName;

} Media;

Media* media_New(char *sPath);
void media_Free(Media *pMedia);
bool media_Seek(Media *pMedia, int64_t nPosition, int nMode);
int64_t media_GetPosition(Media *pMedia);
size_t media_Read(Media *pMedia, void *data, size_t nSize);
int64_t media_Skip(Media *pMedia, int64_t nBytes);
char* media_GetFileName(Media *pMedia);

#endif
