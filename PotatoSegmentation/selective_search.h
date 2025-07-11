#ifndef __SELECTIVE_SEARCH_H__
#define __SELECTIVE_SEARCH_H__

#include "image.h"
#include "disjoint_set.h"
#include <stdbool.h>

#define COLOR_BINS 25
#define TEXTURE_BINS 8
#define COLOR_HIST_SIZE (COLOR_BINS * 3)
#define TEXTURE_HIST_SIZE (TEXTURE_BINS * 3)

#define W_COLOR 1.5f
#define W_TEXTURE 0.8f
#define W_SIZE 1.5f
#define W_FILL 1.0f

typedef struct {
    int id;
    int size;
    int min_x, min_y;
    int max_x, max_y;

    float color_hist[COLOR_HIST_SIZE];
    int r[COLOR_BINS], g[COLOR_BINS], b[COLOR_BINS];

    float texture_hist[TEXTURE_HIST_SIZE];
} Region;

typedef struct {
    Region* regions;
    int count;
    int capacity;
    int img_size;
    int* pixel_to_region;
    bool** adjacent;
} RegionList;

typedef struct {
    int region_idx1;
    int region_idx2;
    float similarity;
} Similarity;

typedef struct {
    Similarity* similarities;
    int count;
    int capacity;
} SimilarityList;


void init_region_list(RegionList* rl);
void rl_free(RegionList* rl);
RegionList create_regions(Image* img, DisjointSet* ds);
int rl_merge_regions(RegionList* rl, int idx1, int idx2);
Region merge_regions(const Region* r1, const Region* r2);


SimilarityList calculate_similarity(RegionList* rl);
void add_similarity(RegionList* rl, SimilarityList* sl, int idx1, int idx2);
void remove_similarity_entries(SimilarityList* sl, int idx1, int idx2);
void sl_free(SimilarityList* sl);


float color_similarity(const Region* region1, const Region* region2);
float texture_similarity(const Region* region1, const Region* region2);
float size_similarity(const Region* region1, const Region* region2, int img_size);
float fill_similarity(const Region* region1, const Region* region2, int img_size);


void selective_search_merge(RegionList* rl, DisjointSet* ds, float threshold, int max_merges, float min_size_factor);
Similarity* get_best_similarity(SimilarityList* sl, RegionList* rl, float min_size_factor);

void selective_search_merge_multi_stage(
    RegionList* rl,
    DisjointSet* ds,
    float start_threshold,
    float end_threshold,
    float threshold_step,
    int max_merges_per_stage,
    float min_size_factor
);


#endif // __SELECTIVE_SEARCH_H__
