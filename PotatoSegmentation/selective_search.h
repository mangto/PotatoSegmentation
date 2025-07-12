// selective_search.h (Cleaned up version)

#ifndef __SELECTIVE_SEARCH_H__
#define __SELECTIVE_SEARCH_H__

#include "image.h"
#include "disjoint_set.h"
#include <stdbool.h>

// --- Macro Definitions ---
#define COLOR_BINS 25
#define TEXTURE_BINS 8
#define COLOR_HIST_SIZE (COLOR_BINS * 3)
#define TEXTURE_HIST_SIZE (TEXTURE_BINS * 3)

#define W_COLOR   2.0f
#define W_TEXTURE 0.5f
#define W_SIZE    1.0f
#define W_FILL    1.5f

// --- Structure Definitions ---
typedef struct {
    int id;
    int size;
    int min_x, min_y, max_x, max_y;
    float color_hist[COLOR_HIST_SIZE];
    int r[COLOR_BINS], g[COLOR_BINS], b[COLOR_BINS];
    float texture_hist[TEXTURE_HIST_SIZE];
    float raw_texture_hist[TEXTURE_HIST_SIZE];
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

typedef struct {
    int min_x, min_y, max_x, max_y;
} BoundingBox;

typedef struct {
    BoundingBox* boxes;
    int count;
    int capacity;
} BoundingBoxList;

// Defines the color space types used in the pipeline.
typedef enum {
    COLOR_SPACE_RGB,
    COLOR_SPACE_LAB_L_CHANNEL
} ColorSpaceType;

// --- Function Prototypes ---

// RegionList Functions
void init_region_list(RegionList* rl);
void rl_free(RegionList* rl);
RegionList create_regions(Image* img, DisjointSet* ds);
int rl_merge_regions(RegionList* rl, int idx1, int idx2);
Region merge_regions(Region* r1, Region* r2);

// SimilarityList Functions
void init_similarity_list(SimilarityList* sl, int initial_capacity);
void calculate_similarity(RegionList* rl, SimilarityList* sl);
void add_similarity(RegionList* rl, SimilarityList* sl, int idx1, int idx2);
void remove_similarity_entries(SimilarityList* sl, int idx1, int idx2);
void sl_free(SimilarityList* sl);
Similarity* get_best_similarity(SimilarityList* sl, RegionList* rl, float min_size_factor);

// Similarity Calculation Functions
float color_similarity(Region* region1, Region* region2);
float texture_similarity(Region* region1, Region* region2);
float size_similarity(Region* region1, Region* region2, int img_size);
float fill_similarity(Region* region1, Region* region2, int img_size);

// Main Algorithm
void selective_search_merge(RegionList* rl, DisjointSet* ds, BoundingBoxList* bbl, int max_merges, float min_size_factor);
BoundingBoxList run_selective_search_pipeline(Image* original_img, ColorSpaceType cs_type, float k, float min_size_factor, float iou_threshold);

// BoundingBox Functions
void init_bbox_list(BoundingBoxList* bbl);
void free_bbox_list(BoundingBoxList* bbl);
void add_bbox(BoundingBoxList* bbl, BoundingBox box);

// Post-processing Functions
float calculate_iou(BoundingBox b1, BoundingBox b2);
void non_maximum_suppression(BoundingBoxList* bbl, float iou_threshold);
void filter_nested_boxes(BoundingBoxList* bbl);
void filter_proposals_by_geometry(BoundingBoxList* bbl, int img_width, int img_height);
bool is_box_fully_contained(BoundingBox b1, BoundingBox b2);

// Utility
int count_active_regions(RegionList* rl);

#endif // __SELECTIVE_SEARCH_H__