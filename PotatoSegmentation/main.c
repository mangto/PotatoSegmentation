#include "stb_image.h"
#include "image.h"
#include "image_process.h"
#include "matrix.h"
#include "gbs.h"
#include "selective_search.h"
#include "utils.h"
#include "test.h"

#include <stdio.h>

int main() {

    Image img;
    DisjointSet ds;

    load_image(&img, "test.jpg");

    graph_based_segmentation(&ds, &img, 200.0f, 1.5f);

    RegionList rl = create_regeions(&img, &ds);

    printf("%d\n", rl.count);
    print_region(&rl.regions[100]);

    free_image(&img);
    ds_free(&ds);


    return 0;
}