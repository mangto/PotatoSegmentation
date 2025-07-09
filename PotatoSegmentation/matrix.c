#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

void init_matrix(Matrix* mat, float* values, int* shape, int dims) {

    int total = 1;

    mat->dims = dims;
    mat->shape = malloc(sizeof(int) * dims);

    for (int i = 0; i < dims; i++) {
        mat->shape[i] = shape[i];
        total *= shape[i];
    }

    mat->values = malloc(sizeof(float) * total);

    for (int i = 0; i < total; i++) {
        mat->values[i] = values ? values[i] : 0.0f;
    }

    mat->get = mat_get;
    mat->set = mat_set;

}

float mat_get(Matrix* mat, ...) {
    assert(mat && mat->shape && mat->values);

    va_list args;
    va_start(args, mat);

    int idx = 0;
    int stride = 1;
    for (int d = mat->dims - 1; d >= 0; --d) {
        int i = va_arg(args, int);
        assert(i >= 0 && i < mat->shape[d]);
        idx += i * stride;
        stride *= mat->shape[d];
    }

    va_end(args);

    int total = 1;
    for (int i = 0; i < mat->dims; i++) total *= mat->shape[i];
    assert(idx >= 0 && idx < total);

    //printf("[DEBUG] Accessing mat->values[%d]\n", idx);

    return mat->values[idx];
}


void mat_set(Matrix* mat, float val, ...) {
    assert(mat && mat->shape && mat->values);

    va_list args;
    va_start(args, val);

    int idx = 0;
    int stride = 1;

    for (int d = mat->dims - 1; d >= 0; --d) {
        int i = va_arg(args, int);
        idx += i * stride;
        stride *= mat->shape[d];
    }

    va_end(args);
    mat->values[idx] = val;
}

void free_matrix(Matrix* mat) {
    free(mat->values);
    free(mat->shape);
}

// for debugging
static void print_matrix_recursive(Matrix* mat, int dim_idx, int* current_indices) {
    const char* default_format = "%.2f";

    if (dim_idx == mat->dims) {
        int idx = 0;
        int stride = 1;
        for (int d = mat->dims - 1; d >= 0; --d) {
            idx += current_indices[d] * stride;
            stride *= mat->shape[d];
        }
        printf(default_format, mat->values[idx]);
        printf(" ");
    }
    else {
        if (dim_idx > 0) {
            printf("[");
        }
        for (int i = 0; i < mat->shape[dim_idx]; ++i) {
            current_indices[dim_idx] = i;
            print_matrix_recursive(mat, dim_idx + 1, current_indices);
        }
        if (dim_idx > 0) {
            printf("]");
            if (dim_idx == 1 && mat->dims > 1) {
                printf("\n");
            }
            else if (dim_idx > 1) {
                printf(" ");
            }
        }
    }
}

void print_matrix(Matrix* mat) {
    assert(mat && mat->values && mat->shape);

    printf("Matrix (dims=%d): ", mat->dims);
    for (int i = 0; i < mat->dims; ++i) {
        printf("%d%s", mat->shape[i], i == mat->dims - 1 ? "\n" : " x ");
    }

    int* current_indices = (int*)malloc(sizeof(int) * mat->dims);
    if (!current_indices) {
        fprintf(stderr, "Memory allocation failed for current_indices in print_matrix.\n");
        return;
    }
    const char* default_format = "%.2f";

    if (mat->dims == 0) {
        printf("[Empty Matrix]\n");
    }
    else if (mat->dims == 1) {
        printf("[");
        for (int i = 0; i < mat->shape[0]; ++i) {
            printf(default_format, mat->values[i]);
            if (i < mat->shape[0] - 1) {
                printf(" ");
            }
        }
        printf("]\n");
    }
    else {
        print_matrix_recursive(mat, 0, current_indices);
        printf("\n");
    }

    free(current_indices);
}



int is_same_shape(Matrix* mat1, Matrix* mat2) {
    if (mat1->dims != mat2->dims) { return 0; }

    for (int i = 0; i < mat1->dims; i++) {
        if (mat1->shape[i] != mat2->shape[i]) { return 0; }
    }
    
    return 1;
}

Matrix mat_add(Matrix* mat1, Matrix* mat2) {
    assert(is_same_shape(mat1, mat2));
    
    Matrix result;
    int total = 1;
    
    for (int i = 0; i < mat1->dims; ++i) total *= mat1->shape[i];

    result.dims = mat1->dims;
    result.shape = malloc(sizeof(int) * mat1->dims);
    memcpy(result.shape, mat1->shape, sizeof(int) * mat1->dims);

    result.values = malloc(sizeof(float) * total);
    for (int i = 0; i < total; ++i) {
        result.values[i] = mat1->values[i] + mat2->values[i];
    }

    result.get = mat1->get;
    result.set = mat1->set;

    return result;
}

Matrix mat_sub(Matrix* mat1, Matrix* mat2) {
    assert(is_same_shape(mat1, mat2));

    Matrix result;
    int total = 1;

    for (int i = 0; i < mat1->dims; ++i) total *= mat1->shape[i];

    result.dims = mat1->dims;
    result.shape = malloc(sizeof(int) * mat1->dims);
    memcpy(result.shape, mat1->shape, sizeof(int) * mat1->dims);

    result.values = malloc(sizeof(float) * total);
    for (int i = 0; i < total; ++i) {
        result.values[i] = mat1->values[i] - mat2->values[i];
    }

    result.get = mat1->get;
    result.set = mat1->set;

    return result;
    return ;
}

Matrix mat_dot(Matrix* mat1, Matrix* mat2) {
    assert(mat1 != NULL && mat1->shape != NULL);
    assert(mat2 != NULL && mat2->shape != NULL);
    assert(mat1->dims == 2 && mat2->dims == 2 && "mat_dot currently supports only 2D matrices.");

    int R1 = mat1->shape[0];
    int C1 = mat1->shape[1];
    int R2 = mat2->shape[0];
    int C2 = mat2->shape[1];

    assert(C1 == R2 && "Matrix dimensions for dot product are incompatible (C1 != R2).");

    Matrix result;
    result.dims = 2;
    result.shape = (int*)malloc(sizeof(int) * 2);
    if (!result.shape) {
        fprintf(stderr, "Memory allocation failed for result.shape in mat_dot.\n");
        exit(EXIT_FAILURE);
    }
    result.shape[0] = R1;
    result.shape[1] = C2;

    int total_elements = R1 * C2;
    result.values = (float*)malloc(sizeof(float) * total_elements);
    if (!result.values) {
        fprintf(stderr, "Memory allocation failed for result.values in mat_dot.\n");
        free(result.shape);
        exit(EXIT_FAILURE);
    }

    memset(result.values, 0, sizeof(float) * total_elements);

    for (int i = 0; i < R1; ++i) {
        for (int k = 0; k < C1; ++k) {

            float mat1_ik = mat1->values[i * C1 + k];
            for (int j = 0; j < C2; ++j) {
                result.values[i * C2 + j] += mat1_ik * mat2->values[k * C2 + j];
            }
        }
    }

    result.get = mat_get;
    result.set = mat_set;

    return result;
}

float mat_elemwise_dot_sum(Matrix* a, Matrix* b) {
    assert(a != NULL && b != NULL);
    assert(a->dims == b->dims);

    for (int i = 0; i < a->dims; i++) {
        assert(a->shape[i] == b->shape[i]);
    }

    int total = 1;
    for (int i = 0; i < a->dims; i++) {
        total *= a->shape[i];
    }

    float sum = 0.0f;
    for (int i = 0; i < total; i++) {
        sum += a->values[i] * b->values[i];
    }

    return sum;
}
