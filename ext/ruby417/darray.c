#include <stdlib.h>
#include <string.h> /* memcpy */
#include <math.h> /* log2 */
#include "darray.h"

struct darray *darray_new(unsigned capacity,
                          void (*eltfree)(void *elt),
                          void *(*malloc)(size_t size),
                          void *(*realloc)(void *ptr, size_t new_size),
                          void (*free)(void *ptr)) {
  struct darray *ary = malloc(sizeof(struct darray));

  if (ary) {
    ary->len = 0;
    ary->capacity = 0;
    ary->data = NULL;
    ary->eltfree = eltfree;
    ary->free = free;
    ary->malloc = malloc;
    ary->realloc = realloc;

    if (!darray_resize_if_necessary(ary, capacity)) {
      free(ary);
      return NULL;
    }
  }

  return ary;
}

struct darray *darray_dup(struct darray *ary) {
  struct darray *dup = ary->malloc(sizeof(struct darray));

  if (dup) {
    dup->len = ary->len;
    dup->capacity = ary->len;
    dup->eltfree = ary->eltfree;
    dup->free = ary->free;
    dup->malloc = ary->malloc;
    dup->realloc = ary->realloc;
    dup->data = ary->malloc(sizeof(void*) * ary->len);

    if (!dup->data) {
      ary->free(dup);
      return NULL;
    }

    memcpy(dup->data, ary->data, ary->len * sizeof(void*));
  }

  return dup;
}

void darray_free(struct darray *ary) {
  if (ary) {
    if (ary->eltfree) {
      for(unsigned i = 0; i < ary->len; i++) {
        ary->free(darray_index(ary, i));
      }
    }

    ary->free(ary->data);
    ary->free(ary);
  }
}

int darray_resize_if_necessary(struct darray *ary, unsigned desired_capacity) {
  if (desired_capacity > ary->capacity || ary->capacity == 0) {
    unsigned capacity2 = 2 << (int) log2(desired_capacity | 1);
    void **new_data = ary->realloc(ary->data, capacity2 * sizeof(void*));

    if (new_data) {
      ary->data = new_data;
      ary->capacity = capacity2;
    } else {
      return 0;
    }
  }

  return 1;
}

void *darray_index(struct darray *ary, unsigned idx) {
  if (idx < ary->len) return ary->data[idx];
  return NULL;
}

void *darray_index_set(struct darray *ary, unsigned idx, void *elt) {
  if (idx < ary->len) return ary->data[idx] = elt;
  return NULL;
}

void *darray_remove_fast(struct darray *ary, unsigned idx) {
  void *elt = darray_index(ary, idx);

  if (idx < ary->len) {
    darray_index_set(ary, idx, darray_index(ary, ary->len-1));
    --ary->len;
  }

  return elt;
}

int darray_push(struct darray *ary, void *elt) {
  if (darray_resize_if_necessary(ary, ary->len+1)) {
    darray_index_set(ary, ary->len++, elt);
    return 1;
  }

  return 0;
}

/*
 * This implementation of merge sort uses an optimization suggested in
 * Algorithms, 4 Ed. by Wayne and Sedgewick -- instead of using an auxilliary
 * array, we treat one array as the "read" array, form which elements are read
 * and the other as the "write" array, the one to which the subsort is written.
 * At each step in the recursion, these arrays switch roles. This technique
 * allows us to avoid explicity copying between the auxilliary array and the
 * main, increasing performances *significantly*, at the expense of some
 * complexity.
 */
static void darray_msort_merge(struct darray *read, struct darray *write,
                               unsigned a, unsigned mid, unsigned b, void *data,
                               int (*cmp)(void *a, void *b, void *data)) {
  unsigned x = a, p = a, q = mid;

  while (x < b) {
    if (q >= b || (p < mid && cmp(darray_index(read, p), darray_index(read, q), data) <= 0)) {
      darray_index_set(write, x++, darray_index(read, p++));
    } else {
      darray_index_set(write, x++, darray_index(read, q++));
    }
  }
}

static void darray_insertion_sort(struct darray *read, struct darray *write,
                                  unsigned a, unsigned b, void *data,
                                  int (*cmp)(void *a, void *b, void *data)) {
  unsigned t, p;
  for (t=p=a; p<b; t=++p) {
    void *elt = darray_index(read, p);

    while (t > a && cmp(elt, darray_index(write, t-1), data) < 0) {
      darray_index_set(write, t, darray_index(write, t-1));
      t--;
    }

    darray_index_set(write, t, elt);
  }
}


static void darray_msort_recurse(struct darray *read, struct darray *write,
                                 unsigned a, unsigned b, void *data,
                                 int (*cmp)(void *a, void *b, void *data)) {
  if (b - a <= 16) {
    darray_insertion_sort(read, write, a, b, data, cmp);
  } else {
    unsigned mid = a + (b - a) / 2;

    darray_msort_recurse(write, read, a, mid, data, cmp);
    darray_msort_recurse(write, read, mid, b, data, cmp);

    darray_msort_merge(read, write, a, mid, b, data, cmp);
  }
}

int darray_msort(struct darray *ary, void *data, int (*cmp)(void *a, void *b, void *data)) {
  struct darray *aux = darray_dup(ary);

  if (aux) {
    darray_msort_recurse(aux, ary, 0, ary->len, data, cmp);
    aux->eltfree = NULL;
    darray_free(aux);
    return 1;
  }

  return 0;
}
