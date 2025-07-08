#include "stb_image.h"
#include "image.h"
#include "utils.h"
#include "selective_search.h"

#include <stdio.h>

int image_load_test() {

    printf("==========================================\n");
    printf("=============== Image Load ===============\n");
    printf("\n");

    int success;
    const char* file = "test.jpg";
    Image MyImage;
    
    printf("Target File: '%s'\n", file);

    success = load_image(&MyImage, file);

    if (success == 1) {
        if (MyImage.width != 4032 || MyImage.height != 3024 || MyImage.channels != 3) {
            success = 0;
        }
    }

    if (success == 0) {
        printf("\n");
        printf("=============== Test Failed ==============\n");
        printf("==========================================\n");
        return 0;
    }

    // expected: Width: 4032, Height: 3024, Channels: 3
    printf("Width: %d, Height: %d, Channels: %d\n", MyImage.width, MyImage.height, MyImage.channels);

    // expected: First Pixel (R, G, B): (146, 149, 154)
    if (MyImage.channels >= 3) {
        printf("First Pixel (R, G, B): (%u, %u, %u)\n", MyImage.pixels->r, MyImage.pixels->g, MyImage.pixels->b);
    }


    printf("\n");
    printf("=============== Test Passed ==============\n");
    printf("==========================================\n");

    return 1;
}

int pixel_distance_test() {

    printf("==========================================\n");
    printf("=============== Image Load ===============\n");
    printf("\n");

    float distance;

    Pixel a = { 40, 44, 52 };
    Pixel b = { 106, 76, 158 };
    
    // expected: 128.90306..
    distance = pixel_distance(a, b);
    printf("Pixel Distance (bw. (%d, %d, %d), (%d, %d, %d)): %.2f\n", a.r, a.g, a.b, b.r, b.g, b.b, distance);


    printf("\n");
    printf("=============== Test Passed ==============\n");
    printf("==========================================\n");

    return 1;

}