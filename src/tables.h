/*
 * $Id: tables.h 1.3 1996/05/24 08:30:44 chasan released $
 *
 * Extended module player private resources.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#ifndef __TABLES_H
#define __TABLES_H

/* dwFrequency = 8363*2^((6*12*16*4 - wPeriod)/(12*16*4)) */
static LONG aFrequencyTable[12 * 16 * 4] =
{
    535232, 534749, 534267, 533785, 533303, 532822, 532341, 531861,
    531381, 530902, 530423, 529945, 529466, 528989, 528512, 528035,
    527558, 527083, 526607, 526132, 525657, 525183, 524709, 524236,
    523763, 523291, 522819, 522347, 521876, 521405, 520934, 520465,
    519995, 519526, 519057, 518589, 518121, 517654, 517187, 516720,
    516254, 515788, 515323, 514858, 514394, 513930, 513466, 513003,
    512540, 512078, 511616, 511154, 510693, 510232, 509772, 509312,
    508853, 508394, 507935, 507477, 507019, 506561, 506104, 505648,
    505192, 504736, 504281, 503826, 503371, 502917, 502463, 502010,
    501557, 501105, 500653, 500201, 499750, 499299, 498849, 498399,
    497949, 497500, 497051, 496602, 496154, 495707, 495260, 494813,
    494367, 493921, 493475, 493030, 492585, 492141, 491697, 491253,
    490810, 490367, 489925, 489483, 489041, 488600, 488159, 487719,
    487279, 486839, 486400, 485961, 485523, 485085, 484647, 484210,
    483773, 483337, 482901, 482465, 482030, 481595, 481161, 480727,
    480293, 479860, 479427, 478994, 478562, 478130, 477699, 477268,
    476838, 476407, 475978, 475548, 475119, 474691, 474262, 473834,
    473407, 472980, 472553, 472127, 471701, 471275, 470850, 470426,
    470001, 469577, 469154, 468730, 468307, 467885, 467463, 467041,
    466620, 466199, 465778, 465358, 464938, 464519, 464100, 463681,
    463263, 462845, 462427, 462010, 461593, 461177, 460761, 460345,
    459930, 459515, 459101, 458686, 458273, 457859, 457446, 457033,
    456621, 456209, 455798, 455386, 454976, 454565, 454155, 453745,
    453336, 452927, 452519, 452110, 451702, 451295, 450888, 450481,
    450075, 449669, 449263, 448858, 448453, 448048, 447644, 447240,
    446837, 446434, 446031, 445628, 445226, 444825, 444424, 444023,
    443622, 443222, 442822, 442423, 442023, 441625, 441226, 440828,
    440430, 440033, 439636, 439240, 438843, 438447, 438052, 437657,
    437262, 436867, 436473, 436080, 435686, 435293, 434900, 434508,
    434116, 433725, 433333, 432942, 432552, 432162, 431772, 431382,
    430993, 430604, 430216, 429828, 429440, 429052, 428665, 428279,
    427892, 427506, 427121, 426735, 426350, 425966, 425581, 425198,
    424814, 424431, 424048, 423665, 423283, 422901, 422520, 422139,
    421758, 421377, 420997, 420617, 420238, 419859, 419480, 419102,
    418723, 418346, 417968, 417591, 417215, 416838, 416462, 416086,
    415711, 415336, 414961, 414587, 414213, 413839, 413466, 413093,
    412720, 412348, 411976, 411604, 411233, 410862, 410491, 410121,
    409751, 409381, 409012, 408643, 408274, 407906, 407538, 407171,
    406803, 406436, 406070, 405703, 405337, 404972, 404606, 404241,
    403877, 403512, 403148, 402784, 402421, 402058, 401695, 401333,
    400971, 400609, 400248, 399887, 399526, 399166, 398805, 398446,
    398086, 397727, 397368, 397010, 396652, 396294, 395936, 395579,
    395222, 394866, 394510, 394154, 393798, 393443, 393088, 392733,
    392379, 392025, 391671, 391318, 390965, 390612, 390260, 389908,
    389556, 389205, 388854, 388503, 388152, 387802, 387452, 387103,
    386754, 386405, 386056, 385708, 385360, 385012, 384665, 384318,
    383971, 383625, 383279, 382933, 382587, 382242, 381897, 381553,
    381209, 380865, 380521, 380178, 379835, 379492, 379150, 378808,
    378466, 378125, 377784, 377443, 377102, 376762, 376422, 376083,
    375743, 375404, 375066, 374727, 374389, 374052, 373714, 373377,
    373040, 372704, 372367, 372032, 371696, 371361, 371026, 370691,
    370356, 370022, 369689, 369355, 369022, 368689, 368356, 368024,
    367692, 367360, 367029, 366698, 366367, 366036, 365706, 365376,
    365047, 364717, 364388, 364060, 363731, 363403, 363075, 362748,
    362420, 362094, 361767, 361440, 361114, 360789, 360463, 360138,
    359813, 359489, 359164, 358840, 358516, 358193, 357870, 357547,
    357225, 356902, 356580, 356259, 355937, 355616, 355295, 354975,
    354655, 354335, 354015, 353696, 353376, 353058, 352739, 352421,
    352103, 351785, 351468, 351151, 350834, 350518, 350201, 349886,
    349570, 349255, 348939, 348625, 348310, 347996, 347682, 347368,
    347055, 346742, 346429, 346117, 345804, 345492, 345181, 344869,
    344558, 344247, 343937, 343627, 343317, 343007, 342697, 342388,
    342079, 341771, 341462, 341154, 340847, 340539, 340232, 339925,
    339618, 339312, 339006, 338700, 338394, 338089, 337784, 337479,
    337175, 336871, 336567, 336263, 335960, 335657, 335354, 335052,
    334749, 334447, 334146, 333844, 333543, 333242, 332941, 332641,
    332341, 332041, 331742, 331442, 331143, 330845, 330546, 330248,
    329950, 329652, 329355, 329058, 328761, 328464, 328168, 327872,
    327576, 327281, 326986, 326691, 326396, 326101, 325807, 325513,
    325220, 324926, 324633, 324340, 324048, 323755, 323463, 323171,
    322880, 322589, 322298, 322007, 321716, 321426, 321136, 320846,
    320557, 320268, 319979, 319690, 319402, 319114, 318826, 318538,
    318251, 317964, 317677, 317390, 317104, 316818, 316532, 316247,
    315961, 315676, 315391, 315107, 314823, 314539, 314255, 313971,
    313688, 313405, 313122, 312840, 312558, 312276, 311994, 311713,
    311431, 311150, 310870, 310589, 310309, 310029, 309749, 309470,
    309191, 308912, 308633, 308355, 308077, 307799, 307521, 307244,
    306966, 306690, 306413, 306136, 305860, 305584, 305309, 305033,
    304758, 304483, 304208, 303934, 303660, 303386, 303112, 302839,
    302566, 302293, 302020, 301747, 301475, 301203, 300932, 300660,
    300389, 300118, 299847, 299577, 299306, 299036, 298767, 298497,
    298228, 297959, 297690, 297421, 297153, 296885, 296617, 296350,
    296082, 295815, 295548, 295282, 295015, 294749, 294483, 294218,
    293952, 293687, 293422, 293157, 292893, 292629, 292365, 292101,
    291837, 291574, 291311, 291048, 290786, 290523, 290261, 289999,
    289738, 289476, 289215, 288954, 288694, 288433, 288173, 287913,
    287653, 287394, 287135, 286876, 286617, 286358, 286100, 285842,
    285584, 285326, 285069, 284812, 284555, 284298, 284042, 283785,
    283529, 283273, 283018, 282763, 282508, 282253, 281998, 281744,
    281489, 281236, 280982, 280728, 280475, 280222, 279969, 279717,
    279464, 279212, 278960, 278709, 278457, 278206, 277955, 277704,
    277454, 277204, 276953, 276704, 276454, 276205, 275955, 275706,
    275458, 275209, 274961, 274713, 274465, 274217, 273970, 273723,
    273476, 273229, 272983, 272737, 272491, 272245, 271999, 271754,
    271509, 271264, 271019, 270774, 270530, 270286, 270042, 269799,
    269555, 269312, 269069, 268826, 268584, 268342, 268100, 267858
};

/* wPeriod = 4*16*428*2^(-nNote/(12*16)) */
static USHORT aPeriodTable[12 * 16] =
{
    27392, 27293, 27195, 27097, 26999, 26902, 26805, 26708,
    26612, 26516, 26421, 26326, 26231, 26136, 26042, 25948,
    25855, 25761, 25669, 25576, 25484, 25392, 25301, 25209,
    25119, 25028, 24938, 24848, 24758, 24669, 24580, 24492,
    24403, 24316, 24228, 24141, 24054, 23967, 23881, 23795,
    23709, 23623, 23538, 23453, 23369, 23285, 23201, 23117,
    23034, 22951, 22868, 22786, 22704, 22622, 22540, 22459,
    22378, 22297, 22217, 22137, 22057, 21978, 21899, 21820,
    21741, 21663, 21585, 21507, 21429, 21352, 21275, 21199,
    21122, 21046, 20970, 20895, 20819, 20744, 20670, 20595,
    20521, 20447, 20373, 20300, 20227, 20154, 20081, 20009,
    19937, 19865, 19793, 19722, 19651, 19580, 19509, 19439,
    19369, 19299, 19230, 19160, 19091, 19023, 18954, 18886,
    18818, 18750, 18682, 18615, 18548, 18481, 18414, 18348,
    18282, 18216, 18150, 18085, 18020, 17955, 17890, 17826,
    17762, 17698, 17634, 17570, 17507, 17444, 17381, 17318,
    17256, 17194, 17132, 17070, 17008, 16947, 16886, 16825,
    16765, 16704, 16644, 16584, 16524, 16465, 16405, 16346,
    16287, 16229, 16170, 16112, 16054, 15996, 15938, 15881,
    15824, 15767, 15710, 15653, 15597, 15541, 15485, 15429,
    15373, 15318, 15263, 15208, 15153, 15098, 15044, 14990,
    14936, 14882, 14828, 14775, 14721, 14668, 14616, 14563,
    14510, 14458, 14406, 14354, 14302, 14251, 14199, 14148,
    14097, 14047, 13996, 13945, 13895, 13845, 13795, 13746
};

static UCHAR aSineTable[32] =
{
      0,  24,  49,  74,  97, 120, 141, 161,
    180, 197, 212, 224, 235, 244, 250, 253,
    255, 253, 250, 244, 235, 224, 212, 197,
    180, 161, 141, 120,  97,  74,  49,  24
};

static signed char aRetrigTable[32] =
{
    0, -1, -2, -4, -8, -16, 0, 0, 0, 1, 2, 4, 8, 16, 0, 0,
    16, 16, 16, 16, 16, 16, 11, 8, 16, 16, 16, 16, 16, 16, 24, 32
};

static signed char aAutoVibratoTable[256] =
{
      0, -1, -2, -4, -5, -7, -8,-10,
    -11,-13,-15,-16,-18,-19,-21,-22,
    -23,-25,-26,-28,-29,-31,-32,-33,
    -35,-36,-37,-38,-40,-41,-42,-43,
    -44,-45,-46,-47,-48,-49,-50,-51,
    -52,-53,-54,-55,-55,-56,-57,-58,
    -58,-59,-59,-60,-60,-61,-61,-61,
    -62,-62,-62,-63,-63,-63,-63,-63,
    -63,-63,-63,-63,-63,-63,-62,-62,
    -62,-61,-61,-61,-60,-60,-59,-59,
    -58,-58,-57,-56,-55,-55,-54,-53,
    -52,-51,-50,-49,-48,-47,-46,-45,
    -44,-43,-42,-41,-40,-38,-37,-36,
    -35,-33,-32,-31,-29,-28,-26,-25,
    -23,-22,-21,-19,-18,-16,-15,-13,
    -11,-10, -8, -7, -5, -4, -2, -1,
      0,  2,  3,  5,  6,  8,  9, 11,
     12, 14, 16, 17, 19, 20, 22, 23,
     24, 26, 27, 29, 30, 32, 33, 34,
     36, 37, 38, 39, 41, 42, 43, 44,
     45, 46, 47, 48, 49, 50, 51, 52,
     53, 54, 55, 56, 56, 57, 58, 59,
     59, 60, 60, 61, 61, 62, 62, 62,
     63, 63, 63, 64, 64, 64, 64, 64,
     64, 64, 64, 64, 64, 64, 63, 63,
     63, 62, 62, 62, 61, 61, 60, 60,
     59, 59, 58, 57, 56, 56, 55, 54,
     53, 52, 51, 50, 49, 48, 47, 46,
     45, 44, 43, 42, 41, 39, 38, 37,
     36, 34, 33, 32, 30, 29, 27, 26,
     24, 23, 22, 20, 19, 17, 16, 14,
     12, 11,  9,  8,  6,  5,  3,  2
};

#endif
