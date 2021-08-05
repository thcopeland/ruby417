#include "spec_helper.h"
#include "image.h"

void test_image8_new_with_allocators(void) {
  fprintf(stderr, "Testing image8_new_with_allocators...");

  struct image8 *im;
  while(!(im=image8_new_with_allocators(10, 20, xmalloc, xfree)));
  assert(im->width == 10);
  assert(im->height == 20);
  image8_free(im);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

static void *calloc1(size_t size) {
  return xcalloc(size, 1);
}

void test_image8_usage(void) {
  fprintf(stderr, "Testing image8 usage...");

  struct image8 *im;
  while(!(im=image8_new_with_allocators(20, 20, calloc1, xfree)));
  assert(image8_get(im, 0, 0) == 0);
  assert(image8_get(im, 19, 0) == 0);
  assert(image8_get(im, 16, 5) == 0);
  image8_set(im, 0, 0, 42);
  image8_set(im, 19, 0, 1);
  image8_set(im, 16, 5, 100);
  image8_set(im, 2000, 5, 1); // should catch out of bounds
  assert(image8_get(im, 0, 0) == 42);
  assert(image8_get(im, 19, 0) == 1);
  assert(image8_get(im, 16, 5) == 100);
  assert(image8_get_with_fallback(im, 16, 5, 42) == 100);
  assert(image8_get_with_fallback(im, 2000, 5, 42) == 42);
  assert(image8_get_with_fallback(im, -1, 0, 42) == 42);
  image8_free(im);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_image32_new_with_allocators(void) {
  fprintf(stderr, "Testing image32_new_with_allocators...");

  struct image32 *im;
  while(!(im=image32_new_with_allocators(10, 20, xmalloc, xfree)));
  assert(im->width == 10);
  assert(im->height == 20);
  image32_free(im);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_image32_usage(void) {
  fprintf(stderr, "Testing image32 usage...");

  struct image32 *im;
  while(!(im=image32_new_with_allocators(20, 20, calloc1, xfree)));
  assert(image32_get(im, 0, 0) == 0);
  assert(image32_get(im, 19, 0) == 0);
  assert(image32_get(im, 16, 5) == 0);
  image32_set(im, 0, 0, 1000);
  image32_set(im, 19, 0, 2000);
  image32_set(im, 16, 5, 3000);
  image32_set(im, 2000, 5, 4000); // should catch out of bounds
  assert(image32_get(im, 0, 0) == 1000);
  assert(image32_get(im, 19, 0) == 2000);
  assert(image32_get(im, 16, 5) == 3000);
  assert(image32_get_with_fallback(im, 16, 5, 42) == 3000);
  assert(image32_get_with_fallback(im, 2000, 5, 42) == 42);
  assert(image32_get_with_fallback(im, -1, 0, 42) == 42);
  image32_free(im);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

int main(int argc, char** argv) {
  void (*(tests[]))(void) = {
    test_image8_new_with_allocators,
    test_image8_usage,
    test_image32_new_with_allocators,
    test_image32_usage
  };
  int num = sizeof(tests) / sizeof(tests[0]);
  run_tests(num, tests);
}
