#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "darray.h"

static unsigned allocations = 0, frees = 0;

void *xmalloc(size_t size) {
  if (rand() % 2) return NULL;
  allocations++;
  void *p = malloc(size);
  return p;
}

void *xrealloc(void *ptr, size_t size) {
  if (rand() % 2) return NULL;
  if (!ptr) allocations++;
  void *p = realloc(ptr, size);
  return p;
}

void xfree(void *ptr) {
  frees++;
  return free(ptr);
}

void assert_mem_clean(void) {
  assert(allocations == frees);
}

struct darray *new_test_array(unsigned length, ...) {
  struct darray *array;
  while (!(array=darray_new_with_allocators(0, NULL, xmalloc, xrealloc, xfree)));

  va_list argp;
  va_start(argp, length);
  while (length--) {
    void *val = (void*) (long) va_arg(argp, int);
    while(!darray_push(array, val));
  }
  va_end(argp);
  return array;
}

void assert_darray_eq(struct darray *a, struct darray *b) {
  assert(a->len == b->len);
  for(int i = 0; i < a->len; i++) assert(darray_index(a, i) == darray_index(b, i));
}

void assert_darray_vals(struct darray *ary, ...) {
  va_list argp;
  va_start(argp, ary);
  for(int i = 0; i < ary->len; i++) assert((long) darray_index(ary, i) == (long) va_arg(argp, int));
  va_end(argp);
}

void test_darray_new_with_allocators(void) {
  fprintf(stderr, "Testing darray_new_with_allocators...");

  struct darray *ary;
  while(!(ary=darray_new_with_allocators(5, NULL, xmalloc, xrealloc, xfree)));
  assert(ary->len == 0);
  assert(ary->capacity == 8);
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_darray_dup(void) {
  fprintf(stderr, "Testing darray_dup...");

  struct darray *dup, *ary = new_test_array(4, 1, 2, 3, 4);
  assert_darray_vals(ary, 1, 2, 3, 4);
  while (!(dup=darray_dup(ary)));
  assert_darray_eq(dup, ary);
  assert(dup->capacity == ary->capacity);
  darray_free(dup);
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_darray_free(void) {
  fprintf(stderr, "Testing darray_free...");

  struct darray *ary;
  while(!(ary=darray_new_with_allocators(5, xfree, xmalloc, xrealloc, xfree)));
  for (int i = 0; i < 16; i++) {
    char *str;
    while(!(str=xmalloc(4)));
    sprintf(str, "%i", i);
    while(!darray_push(ary, str));
  }
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_darray_resize_if_necessary(void) {
  fprintf(stderr, "Testing darray_resize_if_necessary...");

  struct darray *ary;
  while(!(ary=darray_new_with_allocators(0, NULL, xmalloc, xrealloc, xfree)));
  assert(ary->capacity == 2);
  while(!darray_resize_if_necessary(ary, 7));
  assert(ary->capacity == 8);
  while(!darray_resize_if_necessary(ary, 8));
  assert(ary->capacity == 8);
  while(!darray_resize_if_necessary(ary, 16));
  assert(ary->capacity == 32);
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_darray_index(void) {
  fprintf(stderr, "Testing darray_index...");

  struct darray *ary = new_test_array(3, 42, -1, 1);
  assert((long) darray_index(ary, 0) == 42);
  assert((long) darray_index(ary, 1) == -1);
  assert((long) darray_index(ary, 2) == 1);
  assert(darray_index(ary, 3) == NULL);
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_darray_index_set(void) {
  fprintf(stderr, "Testing darray_index_set...");

  struct darray *ary = new_test_array(3, 0, 0, 0);
  assert((long) darray_index_set(ary, 0, (void*) 42l) == 42);
  assert((long) darray_index_set(ary, 1, (void*) -1l) == -1);
  assert((long) darray_index_set(ary, 2, (void*) 1l) == 1);
  assert(darray_index_set(ary, 3, (void*) 1l) == NULL);
  assert((long) darray_index(ary, 0) == 42);
  assert((long) darray_index(ary, 1) == -1);
  assert((long) darray_index(ary, 2) == 1);
  assert(darray_index(ary, 3) == NULL);
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_darray_remove_fast(void) {
  fprintf(stderr, "Testing darray_remove_fast...");

  struct darray *ary = new_test_array(4, 47, 2, 9, 7);
  assert(darray_remove_fast(ary, 5) == NULL);
  assert(darray_remove_fast(ary, 0) == (void *) 47l);
  assert_darray_vals(ary, 7, 2, 9);
  assert(ary->len == 3);
  assert(ary->capacity == 4);
  assert(darray_remove_fast(ary, 1) == (void *) 2l);
  assert_darray_vals(ary, 7, 9);
  assert(ary->len == 2);
  assert(ary->capacity == 4);
  assert(darray_remove_fast(ary, 1) == (void *) 9l);
  assert_darray_vals(ary, 7);
  assert(ary->len == 1);
  assert(ary->capacity == 4);
  assert(darray_remove_fast(ary, 0) == (void *) 7l);
  assert(ary->len == 0);
  assert(ary->capacity == 4);
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_darray_push(void) {
  fprintf(stderr, "Testing darray_push...");

  struct darray *ary;
  while(!(ary=darray_new_with_allocators(0, NULL, xmalloc, xrealloc, xfree)));
  while(!darray_push(ary, (void*) 5l));
  while(!darray_push(ary, (void*) 6l));
  while(!darray_push(ary, (void*) 9l));
  assert(ary->len == 3);
  assert(ary->capacity == 4);
  assert_darray_vals(ary, 5, 6, 9);
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

int cmp(void *a, void *b, void *data) {
  return ((long) a - (long) b) * (long) data;
}

void test_darray_msort(void) {
  fprintf(stderr, "Testing darray_msort...");

  struct darray *ary = new_test_array(0);
  while(!darray_msort(ary, (void *) 1l, cmp));
  assert(ary->len == 0);
  darray_free(ary);

  ary = new_test_array(1, 42);
  while(!darray_msort(ary, (void *) 1l, cmp));
  assert_darray_vals(ary, 42);
  darray_free(ary);

  ary = new_test_array(3, 5, -3, 2);
  while(!darray_msort(ary, (void *) 1l, cmp));
  assert_darray_vals(ary, -3, 2, 5);
  darray_free(ary);

  ary = new_test_array(42, -4731, 3119, 1062, -1723, -3059, 3508, -3571, -4710, 1727, -269, 4645, -3872, -4357, -2464, 3886, 2221, -2342, -2975, 2613, -3085, -3117, 4825, -4767, -2931, 4312, 4223, -3112, 545, -2615, -4283, 308, 2828, 417, -917, -2715, -1226, -319, 3984, -583, -2300, 2487, -959);
  while(!darray_msort(ary, (void *) 1l, cmp));
  assert_darray_vals(ary, -4767, -4731, -4710, -4357, -4283, -3872, -3571, -3117, -3112, -3085, -3059, -2975, -2931, -2715, -2615, -2464, -2342, -2300, -1723, -1226, -959, -917, -583, -319, -269, 308, 417, 545, 1062, 1727, 2221, 2487, 2613, 2828, 3119, 3508, 3886, 3984, 4223, 4312, 4645, 4825);

  while(!darray_msort(ary, (void *) -1l, cmp));
  assert_darray_vals(ary, 4825, 4645, 4312, 4223, 3984, 3886, 3508, 3119, 2828, 2613, 2487, 2221, 1727, 1062, 545, 417, 308, -269, -319, -583, -917, -959, -1226, -1723, -2300, -2342, -2464, -2615, -2715, -2931, -2975, -3059, -3085, -3112, -3117, -3571, -3872, -4283, -4357, -4710, -4731, -4767);
  darray_free(ary);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

int main(int argc, char** argv) {
  int seed = time(0);
  fprintf(stderr, "Randomized with seed %i\n", seed);
  srand(seed);

  void (*(tests[]))(void) = {
    test_darray_new_with_allocators,
    test_darray_dup,
    test_darray_free,
    test_darray_resize_if_necessary,
    test_darray_index,
    test_darray_index_set,
    test_darray_push,
    test_darray_remove_fast,
    test_darray_msort
  };
  int num = sizeof(tests) / sizeof(tests[0]);
  while (num) {
    int idx = rand() % num;
    tests[idx]();
    tests[idx] = tests[--num];
  }
}
