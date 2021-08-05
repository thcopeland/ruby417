#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

static unsigned allocations = 0, frees = 0;

void *xmalloc(size_t size) {
  if (rand() % 2) return NULL;
  allocations++;
  return malloc(size);
}

void *xcalloc(size_t num, size_t size) {
  if (rand() % 2) return NULL;
  allocations++;
  return calloc(num, size);
}

void *xrealloc(void *ptr, size_t size) {
  if (rand() % 2) return NULL;
  if (!ptr) allocations++;
  return realloc(ptr, size);
}

void xfree(void *ptr) {
  frees++;
  return free(ptr);
}

void assert_mem_clean(void) {
  assert(allocations == frees);
}

void run_tests(unsigned count, void (**tests)(void)) {
  int seed = time(0);
  fprintf(stderr, "Randomized with seed %i\n", seed);
  srand(seed);

  while (count) {
    int idx = rand() % count;
    tests[idx]();
    tests[idx] = tests[--count];
  }
}
