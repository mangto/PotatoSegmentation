#define STB_IMAGE_IMPLEMENTATION
#include "image.h"
#include "stb_image.h"
#include <stdio.h>

int load_image(Image* img, const char* FilePath) {

    img->pixels = stbi_load(FilePath, &img->width, &img->height, &img->channels, 0);

    if (img->pixels == NULL) {
        printf("'%s' Failed To Load Image!\n", FilePath);
        return 0;
    }

    return 1;
}



void free_image(Image* img) {
    if (img != NULL && img->pixels != NULL) {
        stbi_image_free(img->pixels);
        img->pixels = NULL;
    }
}