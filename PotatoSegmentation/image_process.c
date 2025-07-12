#include "image.h"
#include "matrix.h"
#include "image_process.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

void contrast_stretch(Image* img, float factor) {
    if (factor <= 0.0f) return;

    for (int c = 0; c < 3; c++) {
        int min_val = 255, max_val = 0;

        for (int i = 0; i < img->width * img->height; i++) {
            unsigned char val;
            if (c == 0) val = img->pixels[i].r;
            else if (c == 1) val = img->pixels[i].g;
            else val = img->pixels[i].b;

            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
        }

        int range = max_val - min_val;
        if (range == 0) continue;

        int mid = (max_val + min_val) / 2;
        int new_min = mid - (int)((mid - min_val) * factor);
        int new_max = mid + (int)((max_val - mid) * factor);

        // clamp
        if (new_min < 0) new_min = 0;
        if (new_max > 255) new_max = 255;
        int new_range = new_max - new_min;
        if (new_range == 0) continue;

        for (int i = 0; i < img->width * img->height; i++) {
            unsigned char* target;
            if (c == 0) target = &img->pixels[i].r;
            else if (c == 1) target = &img->pixels[i].g;
            else target = &img->pixels[i].b;

            int stretched = (*target - new_min) * 255 / new_range;
            if (stretched < 0) stretched = 0;
            if (stretched > 255) stretched = 255;
            *target = (unsigned char)stretched;
        }
    }
}


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

HSV rgb_to_hsv(Pixel p) {
    HSV hsv;
    float r = p.r / 255.0f;
    float g = p.g / 255.0f;
    float b = p.b / 255.0f;

    float cmax = fmaxf(r, fmaxf(g, b));
    float cmin = fminf(r, fminf(g, b));
    float delta = cmax - cmin;

    if (delta == 0) {
        hsv.h = 0;
    }
    else if (cmax == r) {
        hsv.h = 60 * fmodf(((g - b) / delta), 6);
    }
    else if (cmax == g) {
        hsv.h = 60 * (((b - r) / delta) + 2);
    }
    else {
        hsv.h = 60 * (((r - g) / delta) + 4);
    }
    if (hsv.h < 0) {
        hsv.h += 360;
    }

    hsv.s = (cmax == 0) ? 0 : (delta / cmax);

    hsv.v = cmax;

    return hsv;
}

void convert_image_to_hsv(Image* src, Image* dst) {
    dst->width = src->width;
    dst->height = src->height;
    dst->channels = src->channels;
    dst->pixels = (Pixel*)malloc(sizeof(Pixel) * src->width * src->height);

    for (int i = 0; i < src->width * src->height; i++) {
        HSV hsv = rgb_to_hsv(src->pixels[i]);

        dst->pixels[i].r = (int)(hsv.h / 360.0f * 255.0f); // H(0-360) -> 0-255
        dst->pixels[i].g = (int)(hsv.s * 255.0f);          // S(0-1) -> 0-255
        dst->pixels[i].b = (int)(hsv.v * 255.0f);          // V(0-1) -> 0-255
    }
}

Lab rgb_to_lab(Pixel p) {
    float r_linear = p.r / 255.0f;
    float g_linear = p.g / 255.0f;
    float b_linear = p.b / 255.0f;

    r_linear = (r_linear > 0.04045f) ? powf((r_linear + 0.055f) / 1.055f, 2.4f) : (r_linear / 12.92f);
    g_linear = (g_linear > 0.04045f) ? powf((g_linear + 0.055f) / 1.055f, 2.4f) : (g_linear / 12.92f);
    b_linear = (b_linear > 0.04045f) ? powf((b_linear + 0.055f) / 1.055f, 2.4f) : (b_linear / 12.92f);

    float x = r_linear * 0.4124f + g_linear * 0.3576f + b_linear * 0.1805f;
    float y = r_linear * 0.2126f + g_linear * 0.7152f + b_linear * 0.0722f;
    float z = r_linear * 0.0193f + g_linear * 0.1192f + b_linear * 0.9505f;

    x /= 0.95047f;
    y /= 1.00000f;
    z /= 1.08883f;

    x = (x > 0.008856f) ? cbrtf(x) : (7.787f * x + 16.0f / 116.0f);
    y = (y > 0.008856f) ? cbrtf(y) : (7.787f * y + 16.0f / 116.0f);
    z = (z > 0.008856f) ? cbrtf(z) : (7.787f * z + 16.0f / 116.0f);

    Lab lab;
    lab.l = (116.0f * y) - 16.0f;
    lab.a = 500.0f * (x - y);
    lab.b = 200.0f * (y - z);

    return lab;
}

void convert_image_to_lab(Image* src, Image* dst) {
    dst->width = src->width;
    dst->height = src->height;
    dst->channels = src->channels;
    dst->pixels = (Pixel*)malloc(sizeof(Pixel) * src->width * src->height);

    for (int i = 0; i < src->width * src->height; i++) {
        Lab lab = rgb_to_lab(src->pixels[i]);

        dst->pixels[i].r = (int)(lab.l * 2.55f);      // L*(0-100) -> 0-255
        dst->pixels[i].g = (int)(lab.a + 128.0f);    // a*(-128-127) -> 0-255
        dst->pixels[i].b = (int)(lab.b + 128.0f);    // b*(-128-127) -> 0-255
    }
}