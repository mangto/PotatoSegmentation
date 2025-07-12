#include "selective_search.h"
#include "disjoint_set.h"
#include "image.h"
#include "utils.h"
#include "gbs.h"
#include "image_process.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct { float mag; float ori; } GradientPixel;
static GradientPixel* calculate_gradients(Image* img, int channel);

static inline unsigned char get_pixel_channel(Pixel* p, int channel) {
	if (channel == 0) return p->r;
	if (channel == 1) return p->g;
	return p->b;
}

void sl_free(SimilarityList* sl) {
	if (sl && sl->similarities) {
		free(sl->similarities);
		sl->similarities = NULL;
	}
}

float color_similarity(Region* region1, Region* region2) {
	float similarity = 0.0f;
	for (int k = 0; k < 75; k++) {
		similarity += fminf(region1->color_hist[k], region2->color_hist[k]);
	}
	return similarity;
}

float texture_similarity(Region* region1, Region* region2) {
	float similarity = 0.0f;
	for (int k = 0; k < 24; k++) {
		similarity += fminf(region1->texture_hist[k], region2->texture_hist[k]);
	}
	return similarity;
}

float size_similarity(Region* region1, Region* region2, int img_size) {
	return 1.0f - (float)(region1->size + region2->size) / (float)img_size;
}

float fill_similarity(Region* region1, Region* region2, int img_size) {
	int x_min = min(region1->min_x, region2->min_x);
	int y_min = min(region1->min_y, region2->min_y);
	int x_max = max(region1->max_x, region2->max_x);
	int y_max = max(region1->max_y, region2->max_y);

	int bbox_area = (x_max - x_min + 1) * (y_max - y_min + 1);
	int total_area = region1->size + region2->size;

	return 1.0f - (float)(bbox_area - total_area) / (float)img_size;
}


void add_similarity(RegionList* rl, SimilarityList* sl, int idx1, int idx2) {
	if (sl->count >= sl->capacity) {
		sl->capacity = (sl->capacity == 0) ? 100 : sl->capacity * 2;
		sl->similarities = (Similarity*)realloc(sl->similarities, sizeof(Similarity) * sl->capacity);
	}

	if (idx1 > idx2) { int temp = idx1; idx1 = idx2; idx2 = temp; }

	Region* region1 = &rl->regions[idx1];
	Region* region2 = &rl->regions[idx2];

	const float w_color = W_COLOR;
	const float w_texture = W_TEXTURE;
	const float w_size = W_SIZE;
	const float w_fill = W_FILL;

	const float max_possible_score = (w_color * 3.0f) + (w_texture * 3.0f) + (w_size * 1.0f) + (w_fill * 1.0f);

	float raw_similarity =
		w_color * color_similarity(region1, region2) +
		w_texture * texture_similarity(region1, region2) +
		w_size * size_similarity(region1, region2, rl->img_size) +
		w_fill * fill_similarity(region1, region2, rl->img_size);

	float normalized_similarity = (max_possible_score > 0) ? raw_similarity / max_possible_score : 0;

	sl->similarities[sl->count].region_idx1 = idx1;
	sl->similarities[sl->count].region_idx2 = idx2;
	sl->similarities[sl->count].similarity = normalized_similarity;
	sl->count++;
}

Similarity* get_best_similarity(SimilarityList* sl, RegionList* rl, float min_size_factor) {
	if (sl->count == 0) return NULL;
	int best_idx = -1;

	for (int i = 0; i < sl->count; i++) {
		int i1 = sl->similarities[i].region_idx1;
		int i2 = sl->similarities[i].region_idx2;
		if (rl->regions[i1].size == 0 || rl->regions[i2].size == 0) continue;

		best_idx = i;
		break;
	}
	if (best_idx == -1) return NULL;

	float best_score = -1.0f;
	for (int i = 0; i < sl->count; i++) {
		int i1 = sl->similarities[i].region_idx1;
		int i2 = sl->similarities[i].region_idx2;
		if (rl->regions[i1].size == 0 || rl->regions[i2].size == 0) continue;

		float sim = sl->similarities[i].similarity;
		int min_size = min(rl->regions[i1].size, rl->regions[i2].size);

		// Boosts similarity for smaller regions.
		float boosted_sim = sim + (1.0f / (1 + min_size)) * min_size_factor;

		if (boosted_sim > best_score) {
			best_score = boosted_sim;
			best_idx = i;
		}
	}
	return &sl->similarities[best_idx];
}



void init_region_list(RegionList* region_list) {
	region_list->capacity = 100;
	region_list->count = 0;
	region_list->regions = (Region*)malloc(sizeof(Region) * region_list->capacity);
	region_list->img_size = 0;
}

int are_regions_adjacent(Region* a, Region* b) {
	if (a->max_x + 1 < b->min_x || b->max_x + 1 < a->min_x) return 0;
	if (a->max_y + 1 < b->min_y || b->max_y + 1 < a->min_y) return 0;
	return 1;
}

RegionList create_regions(Image* img, DisjointSet* ds) {
	int width = img->width;
	int height = img->height;
	int pixel_count = width * height;

	RegionList rl;
	init_region_list(&rl);
	rl.img_size = pixel_count;

	GradientPixel* grad_r = calculate_gradients(img, 0);
	GradientPixel* grad_g = calculate_gradients(img, 1);
	GradientPixel* grad_b = calculate_gradients(img, 2);

	int map_size = ds->count;
	int* parent_to_idx_map = malloc(sizeof(int) * map_size);
	for (int i = 0; i < map_size; i++) parent_to_idx_map[i] = -1;

	int region_count_final = 0;
	for (int i = 0; i < pixel_count; i++) {
		int parent = ds_find(ds, i);
		if (parent_to_idx_map[parent] == -1) {
			parent_to_idx_map[parent] = region_count_final++;
		}
	}

	rl.capacity = region_count_final;
	rl.count = region_count_final;
	rl.regions = realloc(rl.regions, sizeof(Region) * rl.capacity);

	rl.pixel_to_region = malloc(sizeof(int) * pixel_count);

	float* raw_texture_hists = calloc(rl.capacity * 24, sizeof(float));

	for (int i = 0; i < rl.count; i++) {
		Region* region = &rl.regions[i];
		region->size = 0;
		region->min_x = width; region->max_x = 0;
		region->min_y = height; region->max_y = 0;
		for (int k = 0; k < 25; k++) region->r[k] = region->g[k] = region->b[k] = 0;
	}

	for (int i = 0; i < pixel_count; i++) {
		int parent = ds_find(ds, i);
		int idx = parent_to_idx_map[parent];

		rl.pixel_to_region[i] = idx;
		Region* region = &rl.regions[idx];
		Pixel* pixel = &img->pixels[i];
		int x = i % width;
		int y = i / width;

		if (region->size == 0) region->id = parent;

		region->min_x = min(region->min_x, x);
		region->max_x = max(region->max_x, x);
		region->min_y = min(region->min_y, y);
		region->max_y = max(region->max_y, y);

		region->size++;

		region->r[pixel->r * 25 / 256]++;
		region->g[pixel->g * 25 / 256]++;
		region->b[pixel->b * 25 / 256]++;

		int bin;
		bin = (int)(grad_r[i].ori / (2 * M_PI) * 8.0f);
		raw_texture_hists[idx * 24 + (bin % 8)] += grad_r[i].mag;
		bin = (int)(grad_g[i].ori / (2 * M_PI) * 8.0f);
		raw_texture_hists[idx * 24 + 8 + (bin % 8)] += grad_g[i].mag;
		bin = (int)(grad_b[i].ori / (2 * M_PI) * 8.0f);
		raw_texture_hists[idx * 24 + 16 + (bin % 8)] += grad_b[i].mag;
	}

	rl.adjacent = malloc(sizeof(bool*) * rl.count);
	for (int r = 0; r < rl.count; r++) {
		rl.adjacent[r] = calloc(rl.count, sizeof(bool));
	}

	for (int y = 0; y < height - 1; y++) {
		for (int x = 0; x < width - 1; x++) {
			int r1_idx = rl.pixel_to_region[y * width + x];
			int r2_idx = rl.pixel_to_region[y * width + x + 1];
			int r3_idx = rl.pixel_to_region[(y + 1) * width + x];
			if (r1_idx != r2_idx) rl.adjacent[r1_idx][r2_idx] = rl.adjacent[r2_idx][r1_idx] = true;
			if (r1_idx != r3_idx) rl.adjacent[r1_idx][r3_idx] = rl.adjacent[r3_idx][r1_idx] = true;
		}
	}

	for (int i = 0; i < rl.count; i++) {
		Region* region = &rl.regions[i];
		if (region->size == 0) continue;

		for (int j = 0; j < 25; j++) {
			region->color_hist[j] = (float)region->r[j] / region->size;
			region->color_hist[j + 25] = (float)region->g[j] / region->size;
			region->color_hist[j + 50] = (float)region->b[j] / region->size;
		}

		float* raw_hist_ptr = &raw_texture_hists[i * 24];

		for (int k = 0; k < 24; k++) {
			region->raw_texture_hist[k] = raw_hist_ptr[k];
		}

		float total_mag_r = 0, total_mag_g = 0, total_mag_b = 0;
		for (int j = 0; j < 8; j++) total_mag_r += raw_hist_ptr[j];
		for (int j = 0; j < 8; j++) total_mag_g += raw_hist_ptr[j + 8];
		for (int j = 0; j < 8; j++) total_mag_b += raw_hist_ptr[j + 16];

		if (total_mag_r > 0) for (int j = 0; j < 8; j++) region->texture_hist[j] = raw_hist_ptr[j] / total_mag_r;
		else for (int j = 0; j < 8; j++) region->texture_hist[j] = 0;
		if (total_mag_g > 0) for (int j = 0; j < 8; j++) region->texture_hist[j + 8] = raw_hist_ptr[j + 8] / total_mag_g;
		else for (int j = 0; j < 8; j++) region->texture_hist[j + 8] = 0;
		if (total_mag_b > 0) for (int j = 0; j < 8; j++) region->texture_hist[j + 16] = raw_hist_ptr[j + 16] / total_mag_b;
		else for (int j = 0; j < 8; j++) region->texture_hist[j + 16] = 0;
	}

	free(raw_texture_hists);
	free(parent_to_idx_map);
	free(grad_r);
	free(grad_g);
	free(grad_b);
	return rl;
}

void calculate_similarity(RegionList* rl, SimilarityList* sl) {
	for (int i = 0; i < rl->count; i++) {
		// Skips inactive regions (optimization).
		if (rl->regions[i].size == 0) continue;

		for (int j = i + 1; j < rl->count; j++) {
			if (rl->regions[j].size == 0) continue;

			// Calculates similarity only for adjacent regions.
			if (rl->adjacent[i][j]) {
				add_similarity(rl, sl, i, j);
			}
		}
	}
}

void rl_free(RegionList* rl) {
	if (rl) {
		free(rl->regions);
		free(rl->pixel_to_region);
		if (rl->adjacent) {
			for (int i = 0; i < rl->capacity; i++) {
				free(rl->adjacent[i]);
			}
			free(rl->adjacent);
		}
	}
}

Region merge_regions(Region* r1, Region* r2) {
	Region merged;

	merged.id = min(r1->id, r2->id);
	merged.size = r1->size + r2->size;

	merged.min_x = min(r1->min_x, r2->min_x);
	merged.min_y = min(r1->min_y, r2->min_y);
	merged.max_x = max(r1->max_x, r2->max_x);
	merged.max_y = max(r1->max_y, r2->max_y);

	for (int i = 0; i < 25; i++) {
		merged.r[i] = r1->r[i] + r2->r[i];
		merged.g[i] = r1->g[i] + r2->g[i];
		merged.b[i] = r1->b[i] + r2->b[i];
	}

	if (merged.size > 0) {
		for (int i = 0; i < 25; i++) {
			merged.color_hist[i] = (float)merged.r[i] / merged.size;
			merged.color_hist[i + 25] = (float)merged.g[i] / merged.size;
			merged.color_hist[i + 50] = (float)merged.b[i] / merged.size;
		}
	}

	for (int i = 0; i < 24; i++) {
		merged.raw_texture_hist[i] = r1->raw_texture_hist[i] + r2->raw_texture_hist[i];
	}

	float total_mag_r = 0, total_mag_g = 0, total_mag_b = 0;
	for (int i = 0; i < 8; i++) total_mag_r += merged.raw_texture_hist[i];
	for (int i = 0; i < 8; i++) total_mag_g += merged.raw_texture_hist[i + 8];
	for (int i = 0; i < 8; i++) total_mag_b += merged.raw_texture_hist[i + 16];

	if (total_mag_r > 0) for (int i = 0; i < 8; i++) merged.texture_hist[i] = merged.raw_texture_hist[i] / total_mag_r;
	else for (int i = 0; i < 8; i++) merged.texture_hist[i] = 0;

	if (total_mag_g > 0) for (int i = 0; i < 8; i++) merged.texture_hist[i + 8] = merged.raw_texture_hist[i + 8] / total_mag_g;
	else for (int i = 0; i < 8; i++) merged.texture_hist[i + 8] = 0;

	if (total_mag_b > 0) for (int i = 0; i < 8; i++) merged.texture_hist[i + 16] = merged.raw_texture_hist[i + 16] / total_mag_b;
	else for (int i = 0; i < 8; i++) merged.texture_hist[i + 16] = 0;


	return merged;
}

int rl_merge_regions(RegionList* rl, int idx1, int idx2) {
	if (idx1 == idx2) return -1;
	int keep_idx = min(idx1, idx2);
	int remove_idx = max(idx1, idx2);

	rl->regions[keep_idx] = merge_regions(&rl->regions[idx1], &rl->regions[idx2]);
	rl->regions[remove_idx].size = 0;

	for (int i = 0; i < rl->count; i++) {
		if (i == keep_idx || rl->regions[i].size == 0) continue;
		rl->adjacent[keep_idx][i] = rl->adjacent[i][keep_idx] =
			rl->adjacent[idx1][i] || rl->adjacent[idx2][i];
	}

	for (int i = 0; i < rl->count; i++) {
		rl->adjacent[remove_idx][i] = rl->adjacent[i][remove_idx] = false;
	}

	//rl->count--;

	return 0;
}


void remove_similarity_entries(SimilarityList* sl, int idx1, int idx2) {
	int write_idx = 0;
	for (int i = 0; i < sl->count; i++) {
		int r1 = sl->similarities[i].region_idx1;
		int r2 = sl->similarities[i].region_idx2;
		if (r1 == idx1 || r1 == idx2 || r2 == idx1 || r2 == idx2) {
			continue;
		}
		if (write_idx != i) {
			sl->similarities[write_idx] = sl->similarities[i];
		}
		write_idx++;
	}
	sl->count = write_idx;
}


static GradientPixel* calculate_gradients(Image* img, int channel) {
	int width = img->width;
	int height = img->height;
	int pixel_count = width * height;
	GradientPixel* grads = (GradientPixel*)malloc(sizeof(GradientPixel) * pixel_count);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int idx = y * width + x;

			int x_m1 = (x > 0) ? x - 1 : x;
			int x_p1 = (x < width - 1) ? x + 1 : x;
			int y_m1 = (y > 0) ? y - 1 : y;
			int y_p1 = (y < height - 1) ? y + 1 : y;

			float val_x_m1 = (float)get_pixel_channel(&img->pixels[y * width + x_m1], channel);
			float val_x_p1 = (float)get_pixel_channel(&img->pixels[y * width + x_p1], channel);
			float val_y_m1 = (float)get_pixel_channel(&img->pixels[y_m1 * width + x], channel);
			float val_y_p1 = (float)get_pixel_channel(&img->pixels[y_p1 * width + x], channel);

			float gx = val_x_p1 - val_x_m1;
			float gy = val_y_p1 - val_y_m1;

			grads[idx].mag = sqrtf(gx * gx + gy * gy);
			grads[idx].ori = atan2f(gy, gx) + M_PI;
		}
	}
	return grads;
}

int count_active_regions(RegionList* rl) {
	int active_count = 0;
	for (int i = 0; i < rl->count; i++) {
		if (rl->regions[i].size > 0) {
			active_count++;
		}
	}
	return active_count;
}

// Adds BoundingBoxList-related functions.
void init_bbox_list(BoundingBoxList* bbl) {
	bbl->count = 0;
	bbl->capacity = 1024; // Sets an initial capacity.
	bbl->boxes = (BoundingBox*)malloc(sizeof(BoundingBox) * bbl->capacity);
}

void free_bbox_list(BoundingBoxList* bbl) {
	if (bbl && bbl->boxes) {
		free(bbl->boxes);
		bbl->boxes = NULL;
	}
}

void add_bbox(BoundingBoxList* bbl, BoundingBox box) {
	if (bbl->count >= bbl->capacity) {
		bbl->capacity *= 2;
		bbl->boxes = (BoundingBox*)realloc(bbl->boxes, sizeof(BoundingBox) * bbl->capacity);
	}
	bbl->boxes[bbl->count++] = box;
}


// Replaces the selective_search_merge function in selective_search.c with the code below.

void selective_search_merge(RegionList* rl, DisjointSet* ds, BoundingBoxList* bbl, int max_merges, float min_size_factor) {
	if (rl->count < 2) return;

	// 1. Calculates initial similarity between adjacent regions.
	SimilarityList sl;
	init_similarity_list(&sl, rl->count); // init_similarity_list function required
	calculate_similarity(rl, &sl);

	int merge_count = 0;
	int active_regions = count_active_regions(rl);

	// 2. Repeats until there is only one active region or the maximum number of merges is reached.
	while (sl.count > 0 && active_regions > 1 && merge_count < max_merges) {
		// Finds the best pair to merge.
		Similarity* best_sim = get_best_similarity(&sl, rl, min_size_factor);
		if (!best_sim) break;

		int r_idx1 = best_sim->region_idx1;
		int r_idx2 = best_sim->region_idx2;

		// 3. Creates a new bounding box and adds it to the list (new).
		BoundingBox new_box;
		new_box.min_x = min(rl->regions[r_idx1].min_x, rl->regions[r_idx2].min_x);
		new_box.min_y = min(rl->regions[r_idx1].min_y, rl->regions[r_idx2].min_y);
		new_box.max_x = max(rl->regions[r_idx1].max_x, rl->regions[r_idx2].max_x);
		new_box.max_y = max(rl->regions[r_idx1].max_y, rl->regions[r_idx2].max_y);
		add_bbox(bbl, new_box);

		// 4. Merges regions and updates the similarity list.
		ds_union(ds, rl->regions[r_idx1].id, rl->regions[r_idx2].id);

		int keep_idx = min(r_idx1, r_idx2);

		// Removes all similarity entries related to the merged regions.
		remove_similarity_entries(&sl, r_idx1, r_idx2);

		// Merges the regions.
		rl_merge_regions(rl, r_idx1, r_idx2);

		// Calculates and adds new similarities for the newly merged region.
		for (int j = 0; j < rl->count; j++) {
			if (j == keep_idx || rl->regions[j].size == 0) continue;
			if (rl->adjacent[keep_idx][j]) {
				add_similarity(rl, &sl, keep_idx, j);
			}
		}

		merge_count++;
		active_regions--;
	}
	sl_free(&sl);
}

float calculate_iou(BoundingBox b1, BoundingBox b2) {
	int x_left = max(b1.min_x, b2.min_x);
	int y_top = max(b1.min_y, b2.min_y);
	int x_right = min(b1.max_x, b2.max_x);
	int y_bottom = min(b1.max_y, b2.max_y);

	if (x_right < x_left || y_bottom < y_top) {
		return 0.0f;
	}

	int intersection_area = (x_right - x_left) * (y_bottom - y_top);
	int b1_area = (b1.max_x - b1.min_x) * (b1.max_y - b1.min_y);
	int b2_area = (b2.max_x - b2.min_x) * (b2.max_y - b2.min_y);
	float union_area = (float)(b1_area + b2_area - intersection_area);

	return (union_area > 0) ? (intersection_area / union_area) : 0.0f;
}

// Added to the bottom of selective_search.c

// A function that removes duplicate proposal boxes using NMS.
void non_maximum_suppression(BoundingBoxList* bbl, float iou_threshold) {
	if (bbl->count == 0) return;

	// 1. Temporarily assign a score (here, we assume all boxes have the same confidence).
	//    Instead, we use an array to track the suppression status.
	bool* is_suppressed = (bool*)calloc(bbl->count, sizeof(bool));

	// 2. Perform NMS on all pairs of boxes.
	for (int i = 0; i < bbl->count; i++) {
		if (is_suppressed[i]) continue;

		for (int j = i + 1; j < bbl->count; j++) {
			if (is_suppressed[j]) continue;

			float iou = calculate_iou(bbl->boxes[i], bbl->boxes[j]);

			// If the IoU with box i (which has a higher score) is above the threshold, suppress box j.
			if (iou > iou_threshold) {
				is_suppressed[j] = true;
			}
		}
	}

	// 3. Rebuild the BoundingBoxList with only the unsuppressed boxes.
	BoundingBoxList filtered_bbl;
	init_bbox_list(&filtered_bbl);
	for (int i = 0; i < bbl->count; i++) {
		if (!is_suppressed[i]) {
			add_bbox(&filtered_bbl, bbl->boxes[i]);
		}
	}

	// Replace the original list with the filtered list.
	free(bbl->boxes);
	*bbl = filtered_bbl;

	free(is_suppressed);
}

// A function that checks if one box (b2) is fully contained within another (b1).
bool is_box_fully_contained(BoundingBox b1, BoundingBox b2) {
	return b1.min_x <= b2.min_x &&
		b1.min_y <= b2.min_y &&
		b1.max_x >= b2.max_x &&
		b1.max_y >= b2.max_y;
}

// A function that filters out nested boxes.
void filter_nested_boxes(BoundingBoxList* bbl) {
	if (bbl->count < 2) return;

	bool* is_nested = (bool*)calloc(bbl->count, sizeof(bool));
	if (!is_nested) return;

	for (int i = 0; i < bbl->count; i++) {
		for (int j = 0; j < bbl->count; j++) {
			if (i == j) continue;
			// If box j is fully contained within box i, mark box j for removal.
			if (is_box_fully_contained(bbl->boxes[i], bbl->boxes[j])) {
				is_nested[j] = true;
			}
		}
	}

	// Rebuild the list with only the non-nested boxes.
	BoundingBoxList filtered_bbl;
	init_bbox_list(&filtered_bbl);
	for (int i = 0; i < bbl->count; i++) {
		if (!is_nested[i]) {
			add_bbox(&filtered_bbl, bbl->boxes[i]);
		}
	}

	free(bbl->boxes);
	*bbl = filtered_bbl;
	free(is_nested);
}

// Add this entire function to the bottom of selective_search.c.

void filter_proposals_by_geometry(BoundingBoxList* bbl, int img_width, int img_height) {
	BoundingBoxList filtered_bbl;
	init_bbox_list(&filtered_bbl);

	float min_size_ratio = 0.001f;  // Filters out boxes smaller than 0.1% of the total image size.
	float max_size_ratio = 0.95f;   // Filters out boxes larger than 95% of the total image size.
	float max_aspect_ratio = 10.0f; // Filters out boxes with an aspect ratio (width/height or height/width) greater than 10.
	long img_area = (long)img_width * img_height;

	for (int i = 0; i < bbl->count; i++) {
		BoundingBox box = bbl->boxes[i];
		long box_w = box.max_x - box.min_x;
		long box_h = box.max_y - box.min_y;

		if (box_w <= 0 || box_h <= 0) continue;

		// Size filtering
		long box_area = box_w * box_h;
		if ((float)box_area / img_area < min_size_ratio || (float)box_area / img_area > max_size_ratio) {
			continue;
		}

		// Aspect ratio filtering
		float aspect_ratio = (float)box_w / box_h;
		if (aspect_ratio > max_aspect_ratio || aspect_ratio < (1.0f / max_aspect_ratio)) {
			continue;
		}

		add_bbox(&filtered_bbl, box);
	}

	// Replace the original list with the filtered list.
	free(bbl->boxes);
	*bbl = filtered_bbl;
}

// Add or modify in selective_search.c

void init_similarity_list(SimilarityList* sl, int initial_capacity) {
	sl->count = 0;
	sl->capacity = initial_capacity > 0 ? initial_capacity : 1024;
	sl->similarities = (Similarity*)malloc(sizeof(Similarity) * sl->capacity);
}

// Implementation of the Selective Search pipeline function.
BoundingBoxList run_selective_search_pipeline(Image* original_img, ColorSpaceType cs_type, float k, float min_size_factor, float iou_threshold) {
    const char* cs_name = (cs_type == COLOR_SPACE_RGB) ? "RGB" : "Lab";
    printf("\n--- Running Pipeline for Color Space: %s (k=%.1f) ---\n", cs_name, k);

    BoundingBoxList final_proposals;
    init_bbox_list(&final_proposals);

    Image base_img = copy_image(original_img);
    Image color_img; // The color image used for feature extraction.
    Image gbs_img;   // The input image for Graph-Based Segmentation.

    bool conversion_ok = false;
    switch (cs_type) {
    case COLOR_SPACE_RGB:
        color_img = copy_image(&base_img);
        gbs_img = copy_image(&base_img);
        conversion_ok = (color_img.pixels && gbs_img.pixels);
        break;
    case COLOR_SPACE_LAB_L_CHANNEL:
        convert_image_to_lab(&base_img, &color_img);
        gbs_img.width = color_img.width; gbs_img.height = color_img.height; gbs_img.channels = 1;
        gbs_img.pixels = (Pixel*)malloc(sizeof(Pixel) * gbs_img.width * gbs_img.height);
        if (gbs_img.pixels) {
            for (int i = 0; i < gbs_img.width * gbs_img.height; i++) gbs_img.pixels[i].r = color_img.pixels[i].r;
        }
        conversion_ok = (color_img.pixels && gbs_img.pixels);
        break;
    }

    if (!conversion_ok) {
        fprintf(stderr, "Error during image preparation for colorspace %d\n", cs_type);
        free(base_img.pixels);
        return final_proposals;
    }

    // --- GBS, SS, and Filtering (same process for all color spaces) ---
    DisjointSet ds;
    if (cs_type == COLOR_SPACE_LAB_L_CHANNEL) {
        graph_based_segmentation_grayscale(&ds, &gbs_img, k);
    }
    else {
        graph_based_segmentation(&ds, &gbs_img, k, 2.0f);
    }

    RegionList rl = create_regions(&color_img, &ds);
    printf("Before merge: %d active regions\n", count_active_regions(&rl));

    selective_search_merge(&rl, &ds, &final_proposals, 10000, min_size_factor);

    // Free intermediate memory.
    free(base_img.pixels);
    free(color_img.pixels);
    free(gbs_img.pixels);
    ds_free(&ds);
    rl_free(&rl);

    printf("Pipeline for %s finished. Generated %d proposals.\n", cs_name, final_proposals.count);
    return final_proposals;
}