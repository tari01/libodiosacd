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

#ifndef FILTERSETUP_H
#define FILTERSETUP_H

typedef double CTable[256];

typedef struct
{
    CTable *pFilterTable18;
    CTable *pFilterTable116;
    double *lFilterCoefs22;
    double *lFilterCoefs32;

} FilterSetup;

void filtersetup_New(FilterSetup *pFilterSetup);
void filtersetup_Free(FilterSetup *pFilterSetup);
CTable* filtersetup_GetTables18(FilterSetup *pFilterSetup);
CTable* filtersetup_GetTables116(FilterSetup *pFilterSetup);
double* filtersetup_GetCoefs22(FilterSetup *pFilterSetup);
double* filtersetup_GetCoefs32(FilterSetup *pFilterSetup);

#endif
