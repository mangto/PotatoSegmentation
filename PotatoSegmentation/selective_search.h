#ifndef __SELECTIVE_SEARCH_H__
#define __SELECTIVE_SEARCH_H__

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

void superpixel_segmentation(Image* img);

#endif // !__SELECTIVE_SEARCH_H__
