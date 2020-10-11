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

#include "media.h"
#include <stdlib.h>
#include <string.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

Media* media_New(char *sPath)
{
    Media *pMedia = malloc(sizeof(Media));
    pMedia->pFile = fopen(sPath, "r");
    pMedia->sFilePath = sPath;
    pMedia->sFileName = NULL;

    return pMedia;
}

void media_Free(Media *pMedia)
{
    fclose(pMedia->pFile);

    if (pMedia->sFileName)
    {
        free(pMedia->sFileName);
    }

    free(pMedia);
}

bool media_Seek(Media *pMedia, int64_t nPosition, int nMode)
{
    fseek(pMedia->pFile, nPosition, nMode);

    return true;
}

int64_t media_GetPosition(Media *pMedia)
{
    return ftell(pMedia->pFile);
}

size_t media_Read(Media *pMedia, void *lData, size_t nSize)
{
    return fread(lData, 1, nSize, pMedia->pFile);
}

int64_t media_Skip(Media *pMedia, int64_t nBytes)
{
    return fseek(pMedia->pFile, nBytes, SEEK_CUR);
}

char* media_GetFileName(Media *pMedia)
{
    char *pSlashPos = strrchr(pMedia->sFilePath, '/') + 1;
    pMedia->sFileName = strdup(pSlashPos);
    int nLen = strlen(pMedia->sFileName);
    strcpy(&pMedia->sFileName[nLen - 3], "wav");

    return pMedia->sFileName;
}
