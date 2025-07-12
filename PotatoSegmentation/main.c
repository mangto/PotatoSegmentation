#include "stb_image.h"
#include "image.h"
#include "image_process.h"
#include "matrix.h"
#include "gbs.h"
#include "selective_search.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // 1. Load the original image.
    Image original_img;
    if (!load_image(&original_img, "test2.jpg")) { return -1; }
    printf("Image loaded successfully.\n");

    // 2. Generate proposals for each color space.
    BoundingBoxList proposals_rgb = run_selective_search_pipeline(&original_img, COLOR_SPACE_RGB, 500.0f, 2.0f, 0.5f);
    BoundingBoxList proposals_lab = run_selective_search_pipeline(&original_img, COLOR_SPACE_LAB_L_CHANNEL, 500.0f, 2.0f, 0.5f);

    // 3. Combine all proposals into a single list.
    BoundingBoxList all_proposals;
    init_bbox_list(&all_proposals);
    for (int i = 0; i < proposals_rgb.count; i++) add_bbox(&all_proposals, proposals_rgb.boxes[i]);
    for (int i = 0; i < proposals_lab.count; i++) add_bbox(&all_proposals, proposals_lab.boxes[i]);
    printf("\nTotal raw proposals from all colorspaces: %d\n", all_proposals.count);

    // 4. Apply post-processing filters to the combined list.
    non_maximum_suppression(&all_proposals, 0.5f);
    printf("Filtered to %d proposals after NMS.\n", all_proposals.count);

    filter_nested_boxes(&all_proposals);
    printf("Filtered to %d proposals after removing nested boxes.\n", all_proposals.count);

    filter_proposals_by_geometry(&all_proposals, original_img.width, original_img.height);
    printf("Filtered to %d proposals after geometry filtering.\n", all_proposals.count);

    // 5. Visualize the final proposals.
    visualize_bounding_boxes(&original_img, &all_proposals, "proposals_combined_final.bmp");
    printf("\nFinal combined proposals visualized in 'proposals_combined_final.bmp'.\n");

    // 6. Free all allocated resources.
    free(original_img.pixels);
    free_bbox_list(&proposals_rgb);
    free_bbox_list(&proposals_lab);
    free_bbox_list(&all_proposals);

    printf("\nProcess finished successfully.\n");
    return 0;
}