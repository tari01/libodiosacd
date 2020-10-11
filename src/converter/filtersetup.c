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

#include "filtersetup.h"
#include "memory.h"

const double FILTER18COEFS[80] =
{
    -142,
    -651,
    -1997,
    -4882,
    -10198,
    -18819,
    -31226,
    -46942,
    -63892,
    -77830,
    -82099,
    -67999,
    -26010,
    52003,
    169742,
    323000,
    496497,
    662008,
    778827,
    797438,
    666789,
    344848,
    -188729,
    -919845,
    -1789769,
    -2690283,
    -3466610,
    -3929490,
    -3876295,
    -3119266,
    -1517221,
    994203,
    4379191,
    8490255,
    13072043,
    17781609,
    22223533,
    25995570,
    28738430,
    30182209,
    30182209,
    28738430,
    25995570,
    22223533,
    17781609,
    13072043,
    8490255,
    4379191,
    994203,
    -1517221,
    -3119266,
    -3876295,
    -3929490,
    -3466610,
    -2690283,
    -1789769,
    -919845,
    -188729,
    344848,
    666789,
    797438,
    778827,
    662008,
    496497,
    323000,
    169742,
    52003,
    -26010,
    -67999,
    -82099,
    -77830,
    -63892,
    -46942,
    -31226,
    -18819,
    -10198,
    -4882,
    -1997,
    -651,
    -142
};

const double FILTER116COEFS[160] =
{
    -42,
    -102,
    -220,
    -420,
    -739,
    -1220,
    -1914,
    -2878,
    -4171,
    -5851,
    -7967,
    -10555,
    -13625,
    -17154,
    -21075,
    -25266,
    -29539,
    -33636,
    -37219,
    -39874,
    -41114,
    -40390,
    -37108,
    -30659,
    -20450,
    -5948,
    13272,
    37474,
    66704,
    100733,
    139006,
    180597,
    224174,
    267987,
    309866,
    347255,
    377263,
    396750,
    402440,
    391067,
    359534,
    305112,
    225636,
    119722,
    -13034,
    -171854,
    -354614,
    -557713,
    -775985,
    -1002675,
    -1229481,
    -1446662,
    -1643229,
    -1807208,
    -1925973,
    -1986643,
    -1976541,
    -1883674,
    -1697253,
    -1408195,
    -1009619,
    -497293,
    129993,
    870122,
    1717463,
    2662800,
    3693381,
    4793111,
    5942870,
    7120962,
    8303674,
    9465936,
    10582054,
    11626490,
    12574667,
    13403753,
    14093414,
    14626488,
    14989568,
    15173448,
    15173448,
    14989568,
    14626488,
    14093414,
    13403753,
    12574667,
    11626490,
    10582054,
    9465936,
    8303674,
    7120962,
    5942870,
    4793111,
    3693381,
    2662800,
    1717463,
    870122,
    129993,
    -497293,
    -1009619,
    -1408195,
    -1697253,
    -1883674,
    -1976541,
    -1986643,
    -1925973,
    -1807208,
    -1643229,
    -1446662,
    -1229481,
    -1002675,
    -775985,
    -557713,
    -354614,
    -171854,
    -13034,
    119722,
    225636,
    305112,
    359534,
    391067,
    402440,
    396750,
    377263,
    347255,
    309866,
    267987,
    224174,
    180597,
    139006,
    100733,
    66704,
    37474,
    13272,
    -5948,
    -20450,
    -30659,
    -37108,
    -40390,
    -41114,
    -39874,
    -37219,
    -33636,
    -29539,
    -25266,
    -21075,
    -17154,
    -13625,
    -10555,
    -7967,
    -5851,
    -4171,
    -2878,
    -1914,
    -1220,
    -739,
    -420,
    -220,
    -102,
    -42
};

const double FILTER22COEFS[27] =
{
    349146,
    0,
    -2503287,
    0,
    10155531,
    0,
    -30459917,
    0,
    76750087,
    0,
    -185782569,
    0,
    668365690,
    1073741824,
    668365690,
    0,
    -185782569,
    0,
    76750087,
    0,
    -30459917,
    0,
    10155531,
    0,
    -2503287,
    0,
    349146
};

const double FILTER32COEFS[151] =
{
    -5412,
    0,
    10344,
    0,
    -19926,
    0,
    35056,
    0,
    -57881,
    0,
    91092,
    0,
    -138012,
    0,
    202658,
    0,
    -289823,
    0,
    405153,
    0,
    -555217,
    0,
    747573,
    0,
    -990842,
    0,
    1294782,
    0,
    -1670364,
    0,
    2129866,
    0,
    -2687005,
    0,
    3357104,
    0,
    -4157326,
    0,
    5107022,
    0,
    -6228238,
    0,
    7546440,
    0,
    -9091589,
    0,
    10899739,
    0,
    -13015406,
    0,
    15495180,
    0,
    -18413298,
    0,
    21870494,
    0,
    -26008543,
    0,
    31035142,
    0,
    -37268765,
    0,
    45224971,
    0,
    -55796870,
    0,
    70676173,
    0,
    -93495917,
    0,
    133715464,
    0,
    -226044891,
    0,
    682959923,
    1073741824,
    682959923,
    0,
    -226044891,
    0,
    133715464,
    0,
    -93495917,
    0,
    70676173,
    0,
    -55796870,
    0,
    45224971,
    0,
    -37268765,
    0,
    31035142,
    0,
    -26008543,
    0,
    21870494,
    0,
    -18413298,
    0,
    15495180,
    0,
    -13015406,
    0,
    10899739,
    0,
    -9091589,
    0,
    7546440,
    0,
    -6228238,
    0,
    5107022,
    0,
    -4157326,
    0,
    3357104,
    0,
    -2687005,
    0,
    2129866,
    0,
    -1670364,
    0,
    1294782,
    0,
    -990842,
    0,
    747573,
    0,
    -555217,
    0,
    405153,
    0,
    -289823,
    0,
    202658,
    0,
    -138012,
    0,
    91092,
    0,
    -57881,
    0,
    35056,
    0,
    -19926,
    0,
    10344,
    0,
    -5412
};

static int filtersetup_SetTables(const double *lCoefs, const int nLength, const double fGain, CTable *pCTables)
{
    int ctables = (nLength + 7) / 8;

    for (int ct = 0; ct < ctables; ct++)
    {
        int k = nLength - ct * 8;

        if (k > 8)
        {
            k = 8;
        }

        if (k < 0)
        {
            k = 0;
        }

        for (int i = 0; i < 256; i++)
        {
            double cvalue = 0.0;

            for (int j = 0; j < k; j++)
            {
                cvalue += (((i >> (7 - j)) & 1) * 2 - 1) * lCoefs[nLength - 1 - (ct * 8 + j)];
            }

            pCTables[ct][i] = (double)(cvalue * fGain);
        }
    }

    return ctables;
}

static void filtersetup_SetCoefs(const double *lCoefs, const int nLength, const double fGain, double *lCoefsOut)
{
    for (int i = 0; i < nLength; i++)
    {
        lCoefsOut[i] = (double)(lCoefs[nLength - 1 - i] * fGain);
    }
}

void filtersetup_New(FilterSetup *pFilterSetup)
{
    pFilterSetup->pFilterTable18 = NULL;
    pFilterSetup->pFilterTable116 = NULL;
    pFilterSetup->lFilterCoefs22 = NULL;
    pFilterSetup->lFilterCoefs32 = NULL;
}

void filtersetup_Free(FilterSetup *pFilterSetup)
{
    memFree(pFilterSetup->pFilterTable18);
    memFree(pFilterSetup->pFilterTable116);
    memFree(pFilterSetup->lFilterCoefs22);
    memFree(pFilterSetup->lFilterCoefs32);
}

static const double filtersetup_Norm(const int nScale)
{
    return (double)1 / (double)((unsigned int)1 << (31 - nScale));
}

CTable* filtersetup_GetTables18(FilterSetup *pFilterSetup)
{
    if (!pFilterSetup->pFilterTable18)
    {
        pFilterSetup->pFilterTable18 = (CTable*)memAlloc(((80 + 7) / 8) * sizeof(CTable));
        filtersetup_SetTables(FILTER18COEFS, 80, filtersetup_Norm(3), pFilterSetup->pFilterTable18);
    }

    return pFilterSetup->pFilterTable18;
}

CTable* filtersetup_GetTables116(FilterSetup *pFilterSetup)
{
    if (!pFilterSetup->pFilterTable116)
    {
        pFilterSetup->pFilterTable116 = (CTable*)memAlloc(((160 + 7) / 8) * sizeof(CTable));
        filtersetup_SetTables(FILTER116COEFS, 160, filtersetup_Norm(3), pFilterSetup->pFilterTable116);
    }

    return pFilterSetup->pFilterTable116;
}

double* filtersetup_GetCoefs22(FilterSetup *pFilterSetup)
{
    if (!pFilterSetup->lFilterCoefs22)
    {
        pFilterSetup->lFilterCoefs22 = (double*)memAlloc(27 * sizeof(double));
        filtersetup_SetCoefs(FILTER22COEFS, 27, filtersetup_Norm(0), pFilterSetup->lFilterCoefs22);
    }

    return pFilterSetup->lFilterCoefs22;
}

double* filtersetup_GetCoefs32(FilterSetup *pFilterSetup)
{
    if (!pFilterSetup->lFilterCoefs32)
    {
        pFilterSetup->lFilterCoefs32 = (double*)memAlloc(151 * sizeof(double));
        filtersetup_SetCoefs(FILTER32COEFS, 151, filtersetup_Norm(0), pFilterSetup->lFilterCoefs32);
    }

    return pFilterSetup->lFilterCoefs32;
}
