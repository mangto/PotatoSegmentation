#ifndef __IMAGE__PROCESS_H__
#define __IMAGE__PROCESS_H__

#include "image.h"
#include "matrix.h"

#include <stdio.h>

void contrast_stretch(Image* img, float factor);

Matrix grab_img_to_mat(Image* img, int x, int y, int size_x, int size_y);

void grab_rgb(Matrix* img_mat, int* r, int* g, int* b);

void apply_kernel(Image* img, Matrix* kernel);

Matrix create_gaussian_kernel(int size, float sigma);

#endif // !__IMAGE__PROCESS_H__
