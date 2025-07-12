#define STB_IMAGE_IMPLEMENTATION
#include "image.h"
#include "stb_image.h"
#include <stdio.h>
#include <string.h> 

Image copy_image(Image* src) {
    Image dst;
    dst.width = src->width;
    dst.height = src->height;
    dst.channels = src->channels;

    int image_size = src->width * src->height;
    dst.pixels = (Pixel*)malloc(sizeof(Pixel) * image_size);

    if (dst.pixels != NULL) {
        memcpy(dst.pixels, src->pixels, sizeof(Pixel) * image_size);
    }
    return dst;
}

// Get pixel with cord
Pixel get_pixel(Image* img, int x, int y) {
    return img->pixels[y * img->width + x];
}

// Set pixel with cord
void set_pixel(Image* img, int x, int y, Pixel p) {
    img->pixels[y * img->width + x] = p;
}

static int _load_image_raw(RawImage* img, const char* FilePath) {
    /* Load Image as (r g b r g b ...) */

    img->pixels = stbi_load(FilePath, &img->width, &img->height, &img->channels, 0);

    if (img->pixels == NULL) {
        printf("'%s' Failed To Load Image!\n", FilePath);
        return 0;
    }

    return 1;

}

int load_image(Image* img, const char* FilePath) {
    /* Load Image */

    RawImage raw;
    int ImageSize;

    _load_image_raw(&raw, FilePath); // (r g b r g b ...)
    
    ImageSize = raw.width * raw.height;

    img->width = raw.width;
    img->height = raw.height;
    img->channels = raw.channels;
    img->pixels = (Pixel*) malloc(sizeof(Pixel) * ImageSize);

    if (img->pixels == NULL) {
        // Handle memory allocation failure
        free(raw.pixels); // Free the raw pixel data to prevent leak
        return 0;
    }

    for (int i = 0; i < ImageSize; i++) {
        img->pixels[i].r = raw.pixels[3 * i];
        img->pixels[i].g = raw.pixels[3 * i + 1];
        img->pixels[i].b = raw.pixels[3 * i + 2];
    }

    free(raw.pixels);

    return 1;
}


void free_image(Image* img) {
    if (img != NULL && img->pixels != NULL) {
        stbi_image_free(img->pixels);
        img->pixels = NULL;
    }
}

char* pixel_to_string(Pixel pixel) {
    static char str_buffer[20];
    snprintf(str_buffer, sizeof(str_buffer), "(%d, %d, %d)", pixel.r, pixel.g, pixel.b);

    return str_buffer;
}