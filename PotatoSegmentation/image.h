#ifndef __IMAGE_H_
#define __IMAGE_H_

#include <stdio.h>

typedef struct {
	int width;
	int height;
	int channels;
	unsigned char* pixels;
} RawImage;

typedef struct {
	int r;
	int g;
	int b;
} Pixel;

typedef struct {
	int width;
	int height;
	int channels;
	Pixel* pixels;
} Image;

typedef struct {
	float h, s, v;
} HSV;

typedef struct {
	float l, a, b;
} Lab;

Image copy_image(Image* src);

int _load_image_raw(RawImage* img, const char* FilePath);

int load_image(Image* img, const char* FilePath);

void free_image(Image* image);

char* pixel_to_string(Pixel pixel);



#endif // !__IMAGE_H_