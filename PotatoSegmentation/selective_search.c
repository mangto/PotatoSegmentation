#include "selective_search.h"
#include "disjoint_set.h"
#include "image.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

		// 크기가 작을수록 similarity 보정하여 높게
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

SimilarityList calculate_similarity(RegionList* rl) {
	int region_count = rl->count;
	SimilarityList sl;
	sl.capacity = region_count * region_count / 2;
	sl.similarities = malloc(sizeof(Similarity) * sl.capacity);
	sl.count = 0;

	for (int i = 0; i < region_count; i++) {
		for (int j = i + 1; j < region_count; j++) {
			if (!rl->adjacent[i][j]) continue;
			add_similarity(rl, &sl, i, j);
		}
	}
	return sl;
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

	float merged_raw_texture_hist[24];
	for (int i = 0; i < 24; i++) {
		merged_raw_texture_hist[i] = (r1->texture_hist[i] * r1->size) + (r2->texture_hist[i] * r2->size);
	}

	float total_mag_r = 0, total_mag_g = 0, total_mag_b = 0;
	for (int i = 0; i < 8; i++) total_mag_r += merged_raw_texture_hist[i];
	for (int i = 0; i < 8; i++) total_mag_g += merged_raw_texture_hist[i + 8];
	for (int i = 0; i < 8; i++) total_mag_b += merged_raw_texture_hist[i + 16];

	if (total_mag_r > 0) for (int i = 0; i < 8; i++) merged.texture_hist[i] = merged_raw_texture_hist[i] / total_mag_r;
	else for (int i = 0; i < 8; i++) merged.texture_hist[i] = 0;
	if (total_mag_g > 0) for (int i = 0; i < 8; i++) merged.texture_hist[i + 8] = merged_raw_texture_hist[i + 8] / total_mag_g;
	else for (int i = 0; i < 8; i++) merged.texture_hist[i + 8] = 0;
	if (total_mag_b > 0) for (int i = 0; i < 8; i++) merged.texture_hist[i + 16] = merged_raw_texture_hist[i + 16] / total_mag_b;
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

void selective_search_merge(RegionList* rl, DisjointSet* ds, float threshold, int max_merges, float min_size_factor) {
    if (rl->count < 2) return;

    SimilarityList sl = calculate_similarity(rl);
    int merge_count = 0;

    while (sl.count > 0 && rl->count > 1 && merge_count < max_merges) {
        Similarity* best_sim = get_best_similarity(&sl, rl, min_size_factor);
        if (!best_sim || best_sim->similarity < threshold) break;

        int r_idx1 = best_sim->region_idx1;
        int r_idx2 = best_sim->region_idx2;

        int keep_idx = min(r_idx1, r_idx2);
        int remove_idx = max(r_idx1, r_idx2);

        ds_union(ds, rl->regions[r_idx1].id, rl->regions[r_idx2].id);

        remove_similarity_entries(&sl, r_idx1, r_idx2);

        rl_merge_regions(rl, r_idx1, r_idx2);

        for (int j = 0; j < rl->count; j++) {
            if (j == keep_idx) continue;
            if (rl->adjacent[keep_idx][j]) {
                add_similarity(rl, &sl, keep_idx, j);
            }
        }
        merge_count++;
    }

    sl_free(&sl);
}

void selective_search_merge_multi_stage(
	RegionList* rl,
	DisjointSet* ds,
	float start_threshold,
	float end_threshold,
	float threshold_step,
	int max_merges_per_stage,
	float min_size_factor
) {
	float current_threshold = start_threshold;

	while (current_threshold >= end_threshold) {
		selective_search_merge(rl, ds, current_threshold, max_merges_per_stage, min_size_factor);
		current_threshold -= threshold_step;
	}
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