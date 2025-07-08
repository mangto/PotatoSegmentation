#ifndef __IMAGE_H_
#define __IMAGE_H_

#include <stdio.h>

typedef struct {
	int width, height, channels;
	unsigned char* pixels;
} RawImage;

typedef struct {
	int r, g, b;
} Pixel;

typedef struct {
	int width, height, channels;
	Pixel* pixels;
} Image;

int _load_image_raw(RawImage* img, const char* FilePath);

int load_image(Image* img, const char* FilePath);

void free_image(Image* image);

char* pixel_to_string(Pixel pixel);

#endif // !__IMAGE_H_