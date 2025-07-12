#include "disjoint_set.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void ds_init(DisjointSet* ds, int n) {
    assert(ds != NULL);

    ds->count = n;
    ds->parent = malloc(sizeof(int) * n);
    ds->size = malloc(sizeof(int) * n);

    if (ds->parent == NULL || ds->size == NULL) {
        fprintf(stderr, "FATAL ERROR: Memory allocation failed in ds_init for %d elements.\n", n);
        free(ds->parent);
        free(ds->size);
        exit(EXIT_FAILURE); // Using exit because this is a fatal error
    }

    for (int i = 0; i < n; i++) {
        ds->parent[i] = i;
        ds->size[i] = 1;
    }
}


int ds_find(DisjointSet* ds, int x) {
    if (ds->parent[x] != x) {
        ds->parent[x] = ds_find(ds, ds->parent[x]);  // path compression
    }
    return ds->parent[x];
}

void ds_union(DisjointSet* ds, int x, int y) {
    int rx = ds_find(ds, x);
    int ry = ds_find(ds, y);
    if (rx == ry) return;  // already in same set

    if (ds->size[rx] < ds->size[ry]) {
        ds->parent[rx] = ry;
        ds->size[ry] += ds->size[rx];
    }
    else {
        ds->parent[ry] = rx;
        ds->size[rx] += ds->size[ry];
    }
}

void ds_free(DisjointSet* ds) {
    free(ds->parent);
    free(ds->size);
}