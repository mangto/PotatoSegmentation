#include "image.h"
#include "selective_search.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float pixel_distance(Pixel a, Pixel b) {

	float distance;

	distance = sqrt(
		pow(a.r - b.r, 2) + pow(a.g - b.g, 2) + pow(a.b - b.b, 2)
	);

	return distance;
}

void init_edge_list(EdgeList* list, int capacity) {
	list->data = (Edge*)malloc(sizeof(Edge) * capacity);
	if (list->data == NULL) {
		fprintf(stderr, "malloc failed in init_edge_list\n");
		exit(EXIT_FAILURE);
	}
	list->size = 0;
	list->capacity = capacity;
}


void add_edge(EdgeList* list, int start, int end, float weight) {
	if (list == NULL) {
		fprintf(stderr, "Error: list is NULL in add_edge()\n");
		exit(EXIT_FAILURE);
	}

	if (list->data == NULL) {
		fprintf(stderr, "Error: list->data is NULL in add_edge()\n");
		exit(EXIT_FAILURE);
	}

	if (list->size >= list->capacity) {
		list->capacity *= 2;
		Edge* new_data = (Edge*)realloc(list->data, sizeof(Edge) * list->capacity);
		if (new_data == NULL) {
			fprintf(stderr, "Error: realloc failed in add_edge()\n");
			exit(EXIT_FAILURE);
		}
		list->data = new_data;
	}

	list->data[list->size++] = (Edge){ start, end, weight };
}

void build_edge_graph(Image* img, EdgeList* edges) {
	const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
	const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

	int width = img->width;
	int height = img->height;

	int idx, nx, ny, nidx;
	float weight;
	Pixel origin, neighbor;

	// loop for every pixel
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			idx = y * width + x;
			origin = img->pixels[idx];

			for (int i = 0; i < 8; i++) {
				nx = x + dx[i];
				ny = y + dy[i];

				// only when pixel exists
				if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
					nidx = ny * width + nx;
					neighbor = img->pixels[nidx];
					weight = pixel_distance(origin, neighbor);
					add_edge(edges, idx, nidx, weight);
				}

			}
		}
	}

}

void print_edge_list(const EdgeList* list, int count) {
	/*
	Print edge list for debugging
	if cout == -1 -> see all
	*/


	if (count == -1 || count >= list->size ) { count = list->size; }

	printf("=== Edge List ===\n");
	for (int i = 0; i < count; i++) {
		const Edge e = list->data[i];
		printf("[%d]  src=%d  dst=%d  weight=%.6f\n", i, e.start, e.end, e.weight);
	}
	printf("Total edges: %d\n", list->size);
}

void free_edges(EdgeList* edges) {
	free(edges->data);
}

int compare_edge_weight(const void* a, const void* b) {
	const Edge* ea = (const Edge*)a;
	const Edge* eb = (const Edge*)b;
	if (ea->weight < eb->weight) return -1;
	else if (ea->weight > eb->weight) return 1;
	else return 0;
}

void sort_edge_list(EdgeList* list) {
	// sort edges using qsort
	qsort(list->data, list->size, sizeof(Edge), compare_edge_weight);
}


void superpixel_segmentation(Image* img) {
	EdgeList edges;

	init_edge_list(&edges, 100);
	build_edge_graph(img, &edges);
	
	printf("Before: ");
	print_edge_list(&edges, 10);
	
	sort_edge_list(&edges);

	printf("After: ");
	print_edge_list(&edges, 10);

	free_edges(&edges);
}