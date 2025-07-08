#include "stb_image.h"
#include "image.h"
#include "selective_search.h"
#include "utils.h"
#include "test.h"

#include <stdio.h>

int main() {

    Image img;
    //EdgeList edges;

    load_image(&img, "test.jpg");

    superpixel_segmentation(&img);

    //init_edge_list(&edges, 100);
    //build_edge_graph(&img, &edges);
    //sort_edge_list(&edges);
    //print_edge_list(&edges, 10);

    //free_edges(&edges);
    free_image(&img);

    return 0;
}