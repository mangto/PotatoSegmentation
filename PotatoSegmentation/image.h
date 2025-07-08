#ifndef __IMAGE_H_
#define __IMAGE_H_

#include <stdio.h>

typedef struct {
	int width;
	int height;
	int channels;
	unsigned char* pixels;
} Image;


int load_image(Image* img, const char* FilePath);

void free_image(Image* image);

#endif // !__IMAGE_H_