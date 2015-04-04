// Copyright 2014 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Resources definitions.
//
// Automatically generated with:
// make resources


#ifndef CLOUDS_RESOURCES_H_
#define CLOUDS_RESOURCES_H_


#include "stmlib/stmlib.h"



namespace clouds {

typedef uint8_t ResourceId;

extern const float* src_filter_table[];

extern const int16_t* lookup_table_int16_table[];

extern const float* lookup_table_table[];

extern const float src_filter_1x_2_31[];
extern const float src_filter_1x_2_45[];
extern const float src_filter_1x_2_63[];
extern const float src_filter_1x_2_91[];
extern const int16_t lut_db[];
extern const float lut_sin[];
extern const float lut_window[];
extern const float lut_sine_window_4096[];
extern const float lut_cutoff[];
extern const float lut_grain_size[];
extern const float lut_quantized_pitch[];
#define SRC_FILTER_1X_2_31 0
#define SRC_FILTER_1X_2_31_SIZE 31
#define SRC_FILTER_1X_2_45 1
#define SRC_FILTER_1X_2_45_SIZE 45
#define SRC_FILTER_1X_2_63 2
#define SRC_FILTER_1X_2_63_SIZE 63
#define SRC_FILTER_1X_2_91 3
#define SRC_FILTER_1X_2_91_SIZE 91
#define LUT_DB 0
#define LUT_DB_SIZE 257
#define LUT_SIN 0
#define LUT_SIN_SIZE 1281
#define LUT_WINDOW 1
#define LUT_WINDOW_SIZE 4097
#define LUT_SINE_WINDOW_4096 2
#define LUT_SINE_WINDOW_4096_SIZE 4096
#define LUT_CUTOFF 3
#define LUT_CUTOFF_SIZE 257
#define LUT_GRAIN_SIZE 4
#define LUT_GRAIN_SIZE_SIZE 257
#define LUT_QUANTIZED_PITCH 5
#define LUT_QUANTIZED_PITCH_SIZE 1025

const uint16_t lut_raised_cosine[] = {
       0,      2,      9,     22,
      39,     61,     88,    120,
     157,    199,    246,    298,
     354,    416,    482,    553,
     629,    710,    796,    886,
     982,   1082,   1186,   1296,
    1410,   1530,   1653,   1782,
    1915,   2053,   2195,   2342,
    2494,   2650,   2811,   2976,
    3146,   3320,   3498,   3681,
    3869,   4060,   4256,   4457,
    4661,   4870,   5083,   5300,
    5522,   5747,   5977,   6210,
    6448,   6689,   6935,   7184,
    7437,   7694,   7955,   8220,
    8488,   8760,   9035,   9314,
    9597,   9883,  10172,  10465,
   10762,  11061,  11364,  11670,
   11980,  12292,  12607,  12926,
   13247,  13572,  13899,  14229,
   14562,  14898,  15236,  15578,
   15921,  16267,  16616,  16967,
   17321,  17676,  18034,  18395,
   18757,  19122,  19488,  19857,
   20227,  20600,  20974,  21350,
   21728,  22107,  22488,  22871,
   23255,  23641,  24027,  24416,
   24805,  25196,  25588,  25980,
   26374,  26769,  27165,  27562,
   27959,  28357,  28756,  29155,
   29555,  29956,  30356,  30758,
   31159,  31561,  31963,  32365,
   32767,  33169,  33571,  33973,
   34375,  34776,  35178,  35578,
   35979,  36379,  36778,  37177,
   37575,  37972,  38369,  38765,
   39160,  39554,  39946,  40338,
   40729,  41118,  41507,  41893,
   42279,  42663,  43046,  43427,
   43806,  44184,  44560,  44934,
   45307,  45677,  46046,  46412,
   46777,  47139,  47500,  47858,
   48213,  48567,  48918,  49267,
   49613,  49956,  50298,  50636,
   50972,  51305,  51635,  51962,
   52287,  52608,  52927,  53242,
   53554,  53864,  54170,  54473,
   54772,  55069,  55362,  55651,
   55937,  56220,  56499,  56774,
   57046,  57314,  57579,  57840,
   58097,  58350,  58599,  58845,
   59086,  59324,  59557,  59787,
   60012,  60234,  60451,  60664,
   60873,  61077,  61278,  61474,
   61665,  61853,  62036,  62214,
   62388,  62558,  62723,  62884,
   63040,  63192,  63339,  63481,
   63619,  63752,  63881,  64004,
   64124,  64238,  64348,  64452,
   64552,  64648,  64738,  64824,
   64905,  64981,  65052,  65118,
   65180,  65236,  65288,  65335,
   65377,  65414,  65446,  65473,
   65495,  65512,  65525,  65532,
   65532,
};

}  // namespace clouds

#endif  // CLOUDS_RESOURCES_H_
