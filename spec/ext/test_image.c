#include "spec_helper.h"
#include "image.h"

void test_image_new_with_allocators(void) {
  fprintf(stderr, "Testing image_new_with_allocators...");

  struct image *im;
  while(!(im=image_new_with_allocators(10, 20, 1, xmalloc, xfree)));
  assert(im->width == 10);
  assert(im->height == 20);
  assert(im->eltsize == 1);
  image_free(im);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

static void *calloc1(size_t size) {
  return xcalloc(size, 1);
}

void test_image_usage(void) {
  fprintf(stderr, "Testing image usage...");

  char p42 = 42, p1 = 1, p100 = 100;
  struct image *im;
  while(!(im=image_new_with_allocators(20, 20, 1, calloc1, xfree)));
  assert(*((char*)image_get(im, 0, 0)) == 0);
  assert(*((char*)image_get(im, 19, 0)) == 0);
  assert(*((char*)image_get(im, 16, 5)) == 0);
  image_set(im, 0, 0, &p42);
  image_set(im, 19, 0, &p1);
  image_set(im, 16, 5, &p100);
  image_set(im, 2000, 5, &p100); // should catch out of bounds
  assert(*((char*)image_get(im, 0, 0)) == 42);
  assert(*((char*)image_get(im, 19, 0)) == 1);
  assert(*((char*)image_get(im, 16, 5)) == 100);
  assert(*((char*)image_get_with_fallback(im, 16, 5, &p42)) == 100);
  assert(*((char*)image_get_with_fallback(im, 2000, 5, &p42)) == 42);
  assert(*((char*)image_get_with_fallback(im, -1, 0, &p42)) == 42);
  image_free(im);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

int main(int argc, char** argv) {
  void (*(tests[]))(void) = {
    test_image_new_with_allocators,
    test_image_usage
  };
  int num = sizeof(tests) / sizeof(tests[0]);
  run_tests(num, tests);
}
