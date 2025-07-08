#ifndef __DISJOINT_SET_H__
#define __DISJOINT_SET_H__

typedef struct {
	int* parent;
	int* size;
	int count;
} DisjointSet;

void ds_init(DisjointSet* ds, int n);

int ds_find(DisjointSet* ds, int x);

void ds_union(DisjointSet* ds, int x, int y);

void ds_free(DisjointSet* ds);

#endif // !__DISJOINT_SET_H__
