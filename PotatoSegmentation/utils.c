#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "image.h"
#include "selective_search.h"
#include "disjoint_set.h"

void save_bmp(const char* filename, Pixel* data, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (!f) return;

    int row_padded = (width * 3 + 3) & (~3);
    int image_size = row_padded * height;

    BITMAPFILEHEADER bfh = { 0 };
    BITMAPINFOHEADER bih = { 0 };

    bfh.bfType = 0x4D42;
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + image_size;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height; // top-down
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = 0;
    bih.biSizeImage = image_size;

    fwrite(&bfh, sizeof(bfh), 1, f);
    fwrite(&bih, sizeof(bih), 1, f);

    uint8_t* row = calloc(1, row_padded);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Pixel p = data[y * width + x];
            row[x * 3 + 0] = p.b;
            row[x * 3 + 1] = p.g;
            row[x * 3 + 2] = p.r;
        }
        fwrite(row, 1, row_padded, f);
    }
    free(row);
    fclose(f);
}

#define MAX_LABELS 1000000

void visualize_labels(DisjointSet* ds, const char* filename, int width, int height) {
    int pixel_count = width * height;
    int* labels = malloc(sizeof(int) * pixel_count);
    if (!labels) {
        fprintf(stderr, "malloc failed for labels\n");
        return;
    }

    for (int i = 0; i < pixel_count; i++) {
        labels[i] = ds_find(ds, i);
    }

    Pixel* output = malloc(sizeof(Pixel) * pixel_count);
    if (!output) {
        fprintf(stderr, "malloc failed for output\n");
        free(labels);
        return;
    }

    Pixel* label_colors = malloc(sizeof(Pixel) * MAX_LABELS);
    if (!label_colors) {
        fprintf(stderr, "malloc failed for label_colors\n");
        free(labels);
        free(output);
        return;
    }

    bool* label_assigned = calloc(MAX_LABELS, sizeof(bool));
    if (!label_assigned) {
        fprintf(stderr, "calloc failed for label_assigned\n");
        free(labels);
        free(output);
        free(label_colors);
        return;
    }
    
    ////////////////////////////////////////////////////////////////

    srand((unsigned int)time(NULL));

    for (int i = 0; i < pixel_count; i++) {
        int label = labels[i];
        if (label < 0 || label >= MAX_LABELS) {
            fprintf(stderr, "Warning: invalid label %d at pixel %d\n", label, i);
            label = 0;
        }
        if (!label_assigned[label]) {
            label_colors[label].r = rand() % 256;
            label_colors[label].g = rand() % 256;
            label_colors[label].b = rand() % 256;
            label_assigned[label] = true;
        }
        output[i] = label_colors[label];
    }

    save_bmp(filename, output, width, height);

    free(labels);
    free(output);
    free(label_colors);
    free(label_assigned);
}

void visualize_regions(RegionList* rl, int width, int height, const char* filename) {
    Pixel* output = malloc(sizeof(Pixel) * width * height);
    if (!output) return;

    int* label_map = malloc(sizeof(int) * width * height);
    if (!label_map) {
        free(output);
        return;
    }

    for (int i = 0; i < width * height; i++) label_map[i] = -1;

    Pixel* region_colors = malloc(sizeof(Pixel) * rl->count);
    if (!region_colors) {
        free(output);
        free(label_map);
        return;
    }

    srand((unsigned int)time(NULL));
    for (int i = 0; i < rl->count; i++) {
        region_colors[i].r = rand() % 256;
        region_colors[i].g = rand() % 256;
        region_colors[i].b = rand() % 256;
    }

    for (int i = 0; i < rl->count; i++) {
        Region* r = &rl->regions[i];
        for (int y = r->min_y; y <= r->max_y; y++) {
            for (int x = r->min_x; x <= r->max_x; x++) {
                int idx = y * width + x;
                if (idx < 0 || idx >= width * height) continue;
                label_map[idx] = i;
            }
        }
    }

    for (int i = 0; i < width * height; i++) {
        int label = label_map[i];
        if (label >= 0 && label < rl->count) {
            output[i] = region_colors[label];
        }
        else {
            output[i].r = 0;
            output[i].g = 0;
            output[i].b = 0;
        }
    }

    save_bmp(filename, output, width, height);

    free(output);
    free(label_map);
    free(region_colors);
}



void list_files_in_current_dir() {
    WIN32_FIND_DATAA findData;

    HANDLE hFind = INVALID_HANDLE_VALUE;

    hFind = FindFirstFileA(".\\*", &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error: Cannot find folder.\n");
        return;
    }

    printf("--- File List ---\n");

    do {
        printf("%s\n", findData.cFileName);
    } while (FindNextFileA(hFind, &findData) != 0);

    printf("--------------------------\n");

    FindClose(hFind);

}

void print_array_int(int* arr, int size) {
    printf("[");
    for (int i = 0; i < size; i++) {
        printf("%d", arr[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}

void print_array_float(float* arr, int size) {
    printf("[");
    for (int i = 0; i < size; i++) {
        printf("%.2f", arr[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}