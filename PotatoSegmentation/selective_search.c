#include "selective_search.h"
#include "disjoint_set.h"
#include "image.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

float ss_color_similiarity(RegionList* regions, DisjointSet* ds) {
	int region_count = regions->count;

	for (int i = 0; i < region_count; i++) {
		for (int j = i + 1; j < region_count; j++) {
			continue;
		}
	}

	return 0.0f;
}

void init_region_list(RegionList* region_list) {
	region_list->capacity = 100;
	region_list->count = 0;
	region_list->regions = (Region*)malloc(sizeof(Region)*region_list->capacity);
}

void print_region(Region* region) {
	printf("====================\n");
	printf("id : %d\n", region->id);
	printf("min: (%d, %d)\n", region->min_x, region->min_y);
	printf("max: (%d, %d)\n", region->max_x, region->max_y);

	printf("r  : ");
	print_array_int(region->r, 25);

	printf("g  : ");
	print_array_int(region->g, 25);

	printf("b  : ");
	print_array_int(region->b, 25);
	printf("====================\n");
}

RegionList create_regeions(Image* img, DisjointSet* ds) {
	int pixel_count = img->height * img->width;
	int parent, idx, x, y;
	int size = 100;
	int width = img->width;
	int* idx_ptr  = (int*)malloc(sizeof(int) * size);
	RegionList rl;
	Region* region;
	Pixel* pixel;

	for (int i = 0; i < size; i++) { idx_ptr[i] = -1; } // reset to -1

	init_region_list(&rl);

	for (int i = 0; i < pixel_count; i++) {
		x = i % width;
		y = i / width;

		pixel = &img->pixels[i];
		
		parent = ds_find(ds, i);
		// resize
		if (parent >= size) {

			int* temp = (int*)realloc(idx_ptr, sizeof(int) * (parent+1));

			if (temp == NULL) {
				printf("failed to realloc idx_ptr");
				free(idx_ptr);
				return rl;
			}

			idx_ptr = temp;

			for (int i = size; i < parent+1; i++) { idx_ptr[i] = -1; } // reset new region to -1

			size = parent;
		}

		idx = idx_ptr[parent];

		if (idx == -1) {
			region = &rl.regions[rl.count];

			region->id = parent;
			region->min_x = x;
			region->max_x = x;
			region->min_y = y;
			region->max_y = y;

			for (int i = 0; i < 25; i++) {
				region->r[i] = 0;
				region->g[i] = 0;
				region->b[i] = 0;
			}

			idx_ptr[parent] = rl.count;
			idx = rl.count;
			rl.count++;

			if (rl.count >= rl.capacity) {
				Region* temp = (Region*)realloc(rl.regions, sizeof(Region)*rl.capacity * 2);

				if (temp == NULL) {
					printf("failed to realloc rl.regions");
					free(rl.regions);
					return rl;
				}
				rl.capacity *= 2;
				rl.regions = temp;
			}
		}

		region = &rl.regions[idx];

		region->size++;
		
		region->r[pixel->r * 25 / 256]++;
		region->g[pixel->g * 25 / 256]++;
		region->b[pixel->b * 25 / 256]++;

		if (region->min_x > x) { region->min_x = x; }
		else if (region->max_x < x) { region->max_x = x; }

		if (region->min_y > y) { region->min_y = y; }
		else if (region->max_y < y) { region->max_y = y; }

	}
	free(idx_ptr);

	return rl;
}


