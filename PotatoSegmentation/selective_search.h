#ifndef __SELECTIVE_SEARCH_H__
#define __SELECTIVE_SEARCH_H__

#include "image.h"
#include "disjoint_set.h"

typedef struct {
    int id;
    int size;
    int min_x, min_y;
    int max_x, max_y;
    float centroid_x, centroid_y;
    float* color_hist[75];

    int r[25];
    int g[25];
    int b[25];
} Region;

typedef struct {
    Region* regions;
    int count;
    int capacity;
} RegionList;


float ss_color_similiarity(Image* img, DisjointSet* ds);

void init_region_list(RegionList* region_list);

void print_region(Region* region);

RegionList create_regeions(Image* img, DisjointSet* ds);

#endif // !__SELECTIVE_SEARCH_H__
