#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

#include "disjoint_set.h"
#include "selective_search.h"

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

void list_files_in_current_dir();

void save_bmp(const char* filename, Pixel* data, int width, int height);

void visualize_labels(DisjointSet* ds, const char* filename, int width, int height);

void visualize_regions(RegionList* rl, int width, int height, const char* filename);

void print_array_int(int* arr, int size);

void print_array_float(float* arr, int size);

#endif // !__UTILS_H__
