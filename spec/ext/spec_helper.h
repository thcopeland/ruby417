#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "ruby417.c"

static unsigned allocations = 0, frees = 0, allocation_success_odds = 512;

void *xmalloc(size_t size) {
  if ((rand() & 1023) > allocation_success_odds) return NULL;
  allocations++;
  return malloc(size);
}

void *xcalloc(size_t num, size_t size) {
  if ((rand() & 1023) > allocation_success_odds) return NULL;
  allocations++;
  return calloc(num, size);
}

void *xrealloc(void *ptr, size_t size) {
  if ((rand() & 1023) > allocation_success_odds) return NULL;
  if (!ptr) allocations++;
  return realloc(ptr, size);
}

void xfree(void *ptr) {
  if (ptr) frees++;
  free(ptr);
}

void set_allocation_success_chance(double prob) {
  allocation_success_odds = (unsigned) (1023*prob);
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

char *load_fixture_data(char *filename, unsigned size) {
  static const char prefix[] = "../fixtures/";
  char *data = malloc(size);
  char *full_path = malloc(strlen(prefix) + strlen(filename) + 1);
  sprintf(full_path, "%s%s", prefix, filename);
  FILE *f = fopen(full_path, "rb");
  if (!f) {
    fprintf(stderr, "ERR: unable to read fixture %s (path %s)\n", filename, full_path);
    exit(1);
  }
  if (fread(data, 1, size, f) != size) {
    fprintf(stderr, "ERR: unexpected EOF while reading fixture %s (expected %i)\n", filename, size);
    exit(1);
  }
  fclose(f);
  free(full_path);
  return data;
}

struct image8 *load_image_fixture(char *filename) {
  struct image8 *img = malloc(sizeof(*img));
  unsigned int width, height;
  sscanf(filename, "%ux%u", &width, &height);
  img->width = width;
  img->height = height;
  img->free = free;
  img->data = (unsigned char *) load_fixture_data(filename, width*height);
  return img;
}
