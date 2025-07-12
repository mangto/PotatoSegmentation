#ifndef __UTILS_H__
#define __UTILS_H__

#include <windows.h>

#include "disjoint_set.h"
#include "selective_search.h"


// Function prototypes
void list_files_in_current_dir();
void save_bmp(const char* filename, Pixel* data, int width, int height);
void visualize_labels(DisjointSet* ds, const char* filename, int width, int height);
void visualize_regions(RegionList* rl, int width, int height, const char* filename);
void visualize_bounding_boxes(Image* img, BoundingBoxList* bbl, const char* filename);
void print_array_int(int* arr, int size);
void print_array_float(float* arr, int size);
void draw_rectangle(Pixel* data, int width, int height, BoundingBox box, Pixel color);

#endif // !__UTILS_H__
