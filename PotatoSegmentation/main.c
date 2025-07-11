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

    load_image(&img, "test2.jpg");

    //contrast_stretch(&img, 0.8);

    graph_based_segmentation(&ds, &img, 400.0f, 2.0f);

    visualize_labels(&ds, "test_bf_ssm.bmp", img.width, img.height);

    RegionList rl = create_regions(&img, &ds);

    //selective_search_merge(&rl, &ds, 0.8f, 3000, 50.0f);
    
    selective_search_merge_multi_stage(&rl, &ds, 0.9f, 0.2f, 0.1f, 1000, 80.0f);
    
    visualize_labels(&ds, "test_af_ssm.bmp", img.width, img.height);

    free_image(&img);
    ds_free(&ds);
    rl_free(&rl);


    return 0;
}
