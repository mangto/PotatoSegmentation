#ifndef __GBS__
#define __GBS__

#include "image.h"
#include "disjoint_set.h"

#include <stdio.h>

typedef struct {
	int start;
	int end;
	float weight;
} Edge;

typedef struct {
	Edge* data;
	int size;
	int capacity;
} EdgeList;

float pixel_distance(Pixel a, Pixel b);

void init_edge_list(EdgeList* list, int capacity);

void add_edge(EdgeList* list, int start, int end, float weight);

void build_edge_graph(Image* img, EdgeList* edges);

void free_edges(EdgeList* edges);

void print_edge_list(const EdgeList* list, int count);

int compare_edge_weight(const void* a, const void* b);

void sort_edge_list(EdgeList* list);

void merge_components(EdgeList* edges, DisjointSet* ds, int* size, float* internal, float k);

void graph_based_segmentation(DisjointSet* ds, Image* img, float k, float sigma);

#endif // !__GBS__
