#include "stb_image.h"
#include "image.h"
#include "image_process.h"
#include "gbs.h"
#include "matrix.h"
#include "utils.h"
#include "test.h"

#include <stdio.h>

int main() {

    /*Image img;

    load_image(&img, "test2.jpg");

    graph_based_segmentation(&img, 3000.0f);
    free_image(&img);*/

    Matrix mat1;
    float values1[9] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
    int shape1[2] = { 3, 3 };
    init_matrix(&mat1, &values1, &shape1, 2);

    Matrix mat2;
    float values2[9] = { -2, 0, 2, -2, 0, 2, -2, 0, 2 };
    int shape2[2] = { 3, 3 };
    init_matrix(&mat2, &values2, &shape2, 2);

    Matrix mat3 = mat_add(&mat1, &mat2);
    Matrix mat4 = mat_sub(&mat1, &mat2);

    print_matrix(&mat3);
    print_matrix(&mat4);

    free_matrix(&mat1);
    free_matrix(&mat2);
    free_matrix(&mat3);
    free_matrix(&mat4);

    return 0;
}