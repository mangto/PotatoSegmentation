#include "image.h"
#include "matrix.h"
#include "image_process.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

Matrix grab_img_to_mat(Image* img, int x, int y, int size_x, int size_y) {
    Matrix result;
    result.values = NULL;
    result.shape = NULL;

    int mat_size_2d = size_x * size_y;
    int x_half = size_x / 2;
    int y_half = size_y / 2;

    result.dims = 3;
    result.shape = malloc(sizeof(int) * result.dims);
    if (!result.shape) {
        fprintf(stderr, "Error: shape allocation failed.\n");
        return result; 
    }
    result.shape[0] = size_y;
    result.shape[1] = size_x;
    result.shape[2] = img->channels;

    result.values = malloc(sizeof(float) * mat_size_2d * img->channels);
    if (!result.values) {
        fprintf(stderr, "Error: values allocation failed.\n");
        free(result.shape);
        result.shape = NULL;
        return result;
    }

    for (int dy = 0; dy < size_y; dy++) {
        for (int dx = 0; dx < size_x; dx++) {
            int src_x = x + dx - x_half;
            int src_y = y + dy - y_half;
            int src_idx = src_y * img->width + src_x;

            int dst_idx = dy * size_x + dx;

            result.values[dst_idx] = img->pixels[src_idx].r;
            result.values[dst_idx + mat_size_2d] = img->pixels[src_idx].g;
            result.values[dst_idx + mat_size_2d * 2] = img->pixels[src_idx].b;
        }
    }

    result.get = mat_get;
    result.set = mat_set;
    return result;
}

void grab_rgb(Matrix* img_mat, int* r, int* g, int* b) {

	if (img_mat == NULL || img_mat->shape == NULL || img_mat->dims < 2) {
		fprintf(stderr, "Invalid matrix provided to grab_rgb.\n");
		return;
	}
	int mat_size = img_mat->shape[0] * img_mat->shape[1];
	r = malloc(sizeof(int) * mat_size);
	g = malloc(sizeof(int) * mat_size);
	b = malloc(sizeof(int) * mat_size);

	if (r == NULL || g == NULL || b == NULL) {
		fprintf(stderr, "ERROR: Memory allocation failed in grab_rgb.\n");
		free(&r);
		free(&g);
		free(&b);
		return;
	}

	for (int i = 0; i < mat_size; i++) {
		r[i] = img_mat->values[i];
		g[i] = img_mat->values[mat_size + i];
		b[i] = img_mat->values[mat_size * 2 + i];
	}
}

void apply_kernel(Image* img, Matrix* kernel) {
    int width = img->width;
    int height = img->height;
    int kx = kernel->shape[1];
    int ky = kernel->shape[0];
    int kx_half = kx / 2;
    int ky_half = ky / 2;
	int idx;

    Pixel* buffer = malloc(sizeof(Pixel) * width * height);
    if (!buffer) return;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
			idx = y * width + x;

            if (y < ky_half || y >= height - ky_half || x < kx_half || x >= width - kx_half) {
                buffer[idx] = img->pixels[idx];
                continue;
            }
            Matrix patch3D = grab_img_to_mat(img, x, y, kx, ky);
            if (patch3D.values == NULL) continue;

            int patch_size_2d = kx * ky;

            Matrix r_mat, g_mat, b_mat;
            init_matrix(&r_mat, patch3D.values, kernel->shape, kernel->dims);
            init_matrix(&g_mat, patch3D.values + patch_size_2d, kernel->shape, kernel->dims);
            init_matrix(&b_mat, patch3D.values + patch_size_2d * 2, kernel->shape, kernel->dims);

            float sum_r = mat_elemwise_dot_sum(&r_mat, kernel);
            float sum_g = mat_elemwise_dot_sum(&g_mat, kernel);
            float sum_b = mat_elemwise_dot_sum(&b_mat, kernel);

            free_matrix(&patch3D);
            free(r_mat.shape);
            free(g_mat.shape);
            free(b_mat.shape);

            buffer[idx].r = (int)(sum_r > 255 ? 255 : (sum_r < 0 ? 0 : sum_r));
            buffer[idx].g = (int)(sum_g > 255 ? 255 : (sum_g < 0 ? 0 : sum_g));
            buffer[idx].b = (int)(sum_b > 255 ? 255 : (sum_b < 0 ? 0 : sum_b));
        }
    }

    free(img->pixels);
    img->pixels = buffer;
}

Matrix create_gaussian_kernel(int size, float sigma) {
    assert(size % 2 == 1);

    Matrix kernel;
    int shape[] = { size, size };

    kernel.dims = 2;
    kernel.shape = malloc(sizeof(int) * 2);
    kernel.shape[0] = shape[0];
    kernel.shape[1] = shape[1];

    kernel.values = malloc(sizeof(float) * size * size);

    kernel.get = mat_get;
    kernel.set = mat_set;


    int center = size / 2;
    float sum = 0.0f;
    const float PI = 3.14159265358979323846;

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int rel_x = x - center;
            int rel_y = y - center;

            float value = (1.0f / (2.0f * PI * sigma * sigma)) * exp(-(rel_x * rel_x + rel_y * rel_y) / (2.0f * sigma * sigma));

            kernel.values[y * size + x] = value;
            sum += value;
        }
    }

    for (int i = 0; i < size * size; i++) {
        kernel.values[i] /= sum;
    }

    return kernel;
}