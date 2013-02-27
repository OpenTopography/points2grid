#ifndef _GLOBAL_H_
#define _GLOBAL_H_

static const unsigned int OUTPUT_TYPE_MIN = 0x00000001;
static const unsigned int OUTPUT_TYPE_MAX = 0x00000010;
static const unsigned int OUTPUT_TYPE_MEAN = 0x00000100;
static const unsigned int OUTPUT_TYPE_IDW = 0x00001000;
static const unsigned int OUTPUT_TYPE_DEN = 0x00010000;
static const unsigned int OUTPUT_TYPE_ALL = 0x00011111;

enum OUTPUT_FORMAT {
    OUTPUT_FORMAT_ALL = 0,
    OUTPUT_FORMAT_ARC_ASCII,
    OUTPUT_FORMAT_GRID_ASCII,
};

enum INPUT_FORMAT {
    INPUT_ASCII = 0,
    INPUT_LAS = 1,
};

#endif
