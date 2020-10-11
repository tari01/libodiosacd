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

#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <string.h>

static void* memAlloc(size_t nSize)
{
    nSize = 64 * (size_t)((nSize + 63) / 64);
    void *pMemory = aligned_alloc(64, nSize);

    if (pMemory)
    {
        memset(pMemory, 0, nSize);
    }

    return pMemory;
}

static void memFree(void *pMemory)
{
    if (pMemory)
    {
        free(pMemory);
    }
}

#endif
