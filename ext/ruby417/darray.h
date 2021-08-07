#ifndef DARRAY_H
#define DARRAY_H

#include <stddef.h>

struct darray {
  void** data;
  unsigned len;
  unsigned capacity;
  void (*eltfree)(void *elt);
  void *(*malloc)(size_t size);
  void *(*realloc)(void *ptr, size_t new_size);
  void (*free)(void *ptr);
};

struct darray *darray_new(unsigned capacity,
                          void (*eltfree)(void *elt),
                          void *(*malloc)(size_t size),
                          void *(*realloc)(void *ptr, size_t new_size),
                          void (*free)(void *ptr));
struct darray *darray_dup(struct darray *ary);
void darray_free(struct darray *ary);
int darray_resize_if_necessary(struct darray *ary, unsigned desired);
void *darray_index(struct darray *ary, unsigned idx);
void *darray_index_set(struct darray *ary, unsigned idx, void *elt);
void *darray_remove_fast(struct darray *ary, unsigned idx);
int darray_push(struct darray *ary, void *elt);
int darray_msort(struct darray *ary, void *data, int (*cmp)(void *a, void *b, void *data));

#endif
