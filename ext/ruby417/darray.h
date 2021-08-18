#ifndef DARRAY_H
#define DARRAY_H

#include <stddef.h> // size_t
#include <stdbool.h>

struct darray {
  void **data;
  unsigned len;
  unsigned capacity;
  void (*eltfree)(void *elt);
  void *(*malloc)(size_t size);
  void *(*realloc)(void *ptr, size_t new_size);
  void (*free)(void *ptr);
};

static struct darray *darray_new(unsigned capacity,
                                 void (*eltfree)(void *elt),
                                 void *(*malloc)(size_t size),
                                 void *(*realloc)(void *ptr, size_t new_size),
                                 void (*free)(void *ptr));
static struct darray *darray_dup(struct darray *ary);
static void darray_free(struct darray *ary, bool free_elts);
static bool darray_resize_if_necessary(struct darray *ary, unsigned desired);
static void *darray_index(struct darray *ary, unsigned idx);
static void *darray_index_set(struct darray *ary, unsigned idx, void *elt);
static void *darray_remove_fast(struct darray *ary, unsigned idx);
static bool darray_push(struct darray *ary, void *elt);
static void *darray_pop(struct darray *ary);
static bool darray_msort(struct darray *ary, void *data, int (*cmp)(const void *a, const void *b, void *data));
static void darray_qsort(struct darray *ary, void *data, int (*cmp)(const void *a, const void *b, void *data));

#endif
