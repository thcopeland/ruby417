#ifndef DARRAY_H
#define DARRAY_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h> /* memcpy */
#include <math.h> /* log2 */

typedef struct {
  void** data;
  uint32_t len;
  uint32_t capacity;
} DArray;

typedef int (*DArrayCompareFunc)(const void*, const void*, void*);
typedef void (*DArrayFreeFunc)(void*);

static DArray* darray_new(uint32_t);
static DArray* darray_dup(DArray*);
static void darray_free(DArray*, DArrayFreeFunc);
static int darray_resize_if_necessary(DArray*, uint32_t);
static void* darray_index(DArray*, uint32_t);
static void* darray_index_set(DArray*, uint32_t, void*);
static void* darray_remove_fast(DArray*, uint32_t);
static int darray_push(DArray*, void*);
static int darray_msort(DArray*, void*, DArrayCompareFunc);
static void darray_msort_recurse(DArray*, DArray*, uint32_t, uint32_t, void*, DArrayCompareFunc);
static void darray_msort_merge(DArray*, DArray*, uint32_t, uint32_t, uint32_t, void*, DArrayCompareFunc);
static void darray_insertion_sort(DArray*, DArray*, uint32_t, uint32_t, void*, DArrayCompareFunc);

static DArray* darray_new(uint32_t capacity)
{
  DArray* array = malloc(sizeof(DArray));

  if (array) {
    array->len = 0;
    array->capacity = 0;
    array->data = NULL;

    if (!darray_resize_if_necessary(array, capacity)) {
      free(array);
      return NULL;
    }
  }

  return array;
}

static DArray* darray_dup(DArray* array)
{
  DArray* dup = malloc(sizeof(DArray));

  if (dup) {
    dup->len = array->len;
    dup->capacity = array->len;
    dup->data = malloc(sizeof(void*) * array->len);

    if (!dup->data) {
      free(dup);
      return NULL;
    }

    memcpy(dup->data, array->data, array->len * sizeof(void*));
  }

  return dup;
}

static void darray_free(DArray* array, DArrayFreeFunc liberator)
{
  if(array) {
    if (liberator) {
      for(uint32_t i = 0; i < array->len; i++)
        liberator(darray_index(array, i));
    }

    free(array->data);
    free(array);
  }
}

static int darray_resize_if_necessary(DArray* array, uint32_t desired_capacity)
{
  if (desired_capacity > array->capacity || array->capacity == 0) {
    uint32_t capacity2 = 2 << (int) log2(desired_capacity | 1);
    void** new_data = realloc(array->data, capacity2 * sizeof(void*));

    if (new_data) {
      array->data = new_data;
      array->capacity = capacity2;
    } else {
      return 0;
    }
  }

  return 1;
}

static void* darray_index(DArray* array, uint32_t index)
{
  if (index < array->len)
    return array->data[index];

  return NULL;
}

static void* darray_index_set(DArray* array, uint32_t index, void* val)
{
  if (index < array->len)
    return array->data[index] = val;

  return NULL;
}

static int darray_push(DArray* array, void* element)
{
  if (darray_resize_if_necessary(array, array->len+1)) {
    darray_index_set(array, array->len++, element);
    return 1;
  }

  return 0;
}

static void* darray_remove_fast(DArray* array, uint32_t index)
{
  void* elt = darray_index(array, index);

  if (index < array->len) {
    darray_index_set(array, index, darray_index(array, array->len-1));
    --array->len;
  }

  return elt;
}

static int darray_msort(DArray* array, void* data, DArrayCompareFunc cmp)
{
  DArray* aux = darray_dup(array);

  if (aux) {
    darray_msort_recurse(aux, array, 0, array->len, data, cmp);

    darray_free(aux, NULL);

    return 1;
  }

  return 0;
}

static void darray_msort_recurse(DArray* read, DArray* write,
                                 uint32_t a,   uint32_t b,
                                 void* data,   DArrayCompareFunc cmp)
{
  if (b - a <= 16) {
    darray_insertion_sort(read, write, a, b, data, cmp);
  } else {
    uint32_t mid = a + (b - a) / 2;

    darray_msort_recurse(write, read, a, mid, data, cmp);
    darray_msort_recurse(write, read, mid, b, data, cmp);

    darray_msort_merge(read, write, a, mid, b, data, cmp);
  }
}

static void darray_msort_merge(DArray* read, DArray* write,
                               uint32_t a,   uint32_t mid,
                               uint32_t b,   void* data,
                               DArrayCompareFunc cmp)
{
  uint32_t x = a, p = a, q = mid;

  while (x < b) {
    if (q >= b || (p < mid && cmp(darray_index(read, p), darray_index(read, q), data) <= 0)) {
      darray_index_set(write, x++, darray_index(read, p++));
    } else {
      darray_index_set(write, x++, darray_index(read, q++));
    }
  }
}

static void darray_insertion_sort(DArray* read, DArray* write,
                                  uint32_t a,   uint32_t b,
                                  void* data, DArrayCompareFunc cmp)
{
  uint32_t t, p;
  void* elt;

  for (t=p=a; p<b; t=++p) {
    elt = darray_index(read, p);

    while (t > a && cmp(elt, darray_index(write, t-1), data) < 0) {
      darray_index_set(write, t, darray_index(write, t-1));
      t--;
    }

    darray_index_set(write, t, elt);
  }
}

#endif
