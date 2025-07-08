#include "stb_image.h"
#include "image.h"
#include "utils.h"

#include <stdio.h>

int image_load_test() {

    int success;
    const char* file = "test.jpg";
    Image MyImage;
    
    printf("==========================================\n");
    printf("=============== Image Load ===============\n");
    printf("\n");
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
        unsigned char r = MyImage.pixels[0];
        unsigned char g = MyImage.pixels[1];
        unsigned char b = MyImage.pixels[2];
        printf("First Pixel (R, G, B): (%u, %u, %u)\n", r, g, b);

        printf("%\n", MyImage.pixels);
    }


    printf("\n");
    printf("=============== Test Passed ==============\n");
    printf("==========================================\n");

    return 1;
}