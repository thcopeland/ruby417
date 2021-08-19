#include "spec_helper.h"

void test_image8_new(void) {
  fprintf(stderr, "Testing image8_new...");

  struct image8 *im;
  while(!(im=image8_new(10, 20, xmalloc, xfree)));
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
  while(!(im=image8_new(20, 20, calloc1, xfree)));
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
  image8_free(NULL);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_image32_new(void) {
  fprintf(stderr, "Testing image32_new...");

  struct image32 *im;
  while(!(im=image32_new(10, 20, xmalloc, xfree)));
  assert(im->width == 10);
  assert(im->height == 20);
  image32_free(im);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_image32_usage(void) {
  fprintf(stderr, "Testing image32 usage...");

  struct image32 *im;
  while(!(im=image32_new(20, 20, calloc1, xfree)));
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
  image32_free(NULL);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}


void test_point_new(void) {
  fprintf(stderr, "Testing point_new...");

  struct point *p;
  while(!(p=point_new(42, 19, xmalloc)));
  assert(p->x == 42);
  assert(p->y == 19);
  xfree(p);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_region_new(void) {
  fprintf(stderr, "Testing region_new...");

  struct region *reg;
  while(!(reg=region_new(xmalloc, xrealloc, xfree)));
  assert(reg->cx == 0);
  assert(reg->cy == 0);
  assert(reg->area == 0);
  assert(reg->boundary);
  assert(reg->boundary->len == 0);
  assert(reg->boundary->capacity > 0);
  assert(reg->boundary->capacity < 64);
  region_free(reg);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_region_shallow_free(void) {
  fprintf(stderr, "Testing region_shallow_free...");

  struct point p1 = {4, 3}, p2 = {0, 19};
  struct region *reg;
  while(!(reg=region_new(xmalloc, xrealloc, xfree)));
  while(!darray_push(reg->boundary, &p1));
  while(!darray_push(reg->boundary, &p2));
  region_shallow_free(reg);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_region_free(void) {
  fprintf(stderr, "Testing region_free...");

  struct point *p1, *p2;
  while(!(p1=point_new(4, 3, xmalloc)));
  while(!(p2=point_new(0, 19, xmalloc)));
  struct region *reg;
  while(!(reg=region_new(xmalloc, xrealloc, xfree)));
  while(!darray_push(reg->boundary, p1));
  while(!darray_push(reg->boundary, p2));
  region_free(reg);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_union_find(void) {
  fprintf(stderr, "Testing union/find...");

  struct darray *acc;
  while(!(acc=darray_new(0, xfree, xmalloc, xrealloc, xfree)));
  while(!uf_union(acc, 3, 4));
  assert(uf_find(acc, 3) == uf_find(acc, 4));
  while(!uf_union(acc, 5, 17));
  assert(uf_find(acc, 5) == uf_find(acc, 17));
  assert(uf_find(acc, 17) != uf_find(acc, 3));
  while(!uf_union(acc, 3, 17));
  assert(uf_find(acc, 5) == uf_find(acc, 4));
  assert(uf_find(acc, 3) == uf_find(acc, 17));
  darray_free(acc, false);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_image_label_regions(void) {
  fprintf(stderr, "Testing image_label_regions...");

  struct image8 *im = load_image_fixture("32x32_complex_regions.raw");
  struct image32 *labeled;
  while (!(labeled=image_label_regions(im, xmalloc, xrealloc, xfree)));
  assert(image32_get(labeled, 0, 0) == image32_get(labeled, 16, 0));
  assert(image32_get(labeled, 0, 0) == image32_get(labeled, 28, 0));
  assert(image32_get(labeled, 0, 0) == image32_get(labeled, 16, 16));
  assert(image32_get(labeled, 0, 0) == image32_get(labeled, 0, 27));
  assert(image32_get(labeled, 0, 0) != image32_get(labeled, 1, 0));
  assert(image32_get(labeled, 0, 0) != image32_get(labeled, 6, 0));
  assert(image32_get(labeled, 0, 0) != image32_get(labeled, 11, 0));
  assert(image32_get(labeled, 0, 0) != image32_get(labeled, 2, 12));
  assert(image32_get(labeled, 0, 0) != image32_get(labeled, 16, 14));
  assert(image32_get(labeled, 0, 0) != image32_get(labeled, 31, 15));
  assert(image32_get(labeled, 0, 0) != image32_get(labeled, 31, 15));
  assert(image32_get(labeled, 16, 0) != image32_get(labeled, 17, 0));
  assert(image32_get(labeled, 16, 0) == image32_get(labeled, 16, 1));
  assert(image32_get(labeled, 17, 0) != image32_get(labeled, 18, 0));
  assert(image32_get(labeled, 17, 0) == image32_get(labeled, 17, 1));
  assert(image32_get(labeled, 18, 0) != image32_get(labeled, 19, 0));
  assert(image32_get(labeled, 18, 0) == image32_get(labeled, 18, 1));
  assert(image32_get(labeled, 31, 15) != image32_get(labeled, 15, 19));
  assert(image32_get(labeled, 2, 31) == image32_get(labeled, 4, 28));
  assert(image32_get(labeled, 2, 31) == image32_get(labeled, 2, 28));
  assert(image32_get(labeled, 2, 31) == image32_get(labeled, 3, 29));
  assert(image32_get(labeled, 2, 31) != image32_get(labeled, 2, 29));
  image8_free(im);
  image32_free(labeled);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_image_follow_contour(void) {
  fprintf(stderr, "Testing image_follow_contour...");

  struct image8 *im = load_image_fixture("32x32_solitary_circle.raw");
  struct darray *boundary;
  while(!(boundary=darray_new(32, xfree, xmalloc, xrealloc, xfree)));
  set_allocation_success_chance(0.85);
  while(!image_follow_contour(im, boundary, 15, 8)) {
    // reset boundary array
    for (unsigned i=0; i<boundary->len; i++) boundary->eltfree(darray_index(boundary, i));
    boundary->len = 0;
  }
  set_allocation_success_chance(0.5);
  assert(boundary->len == 40);
  image8_free(im);
  darray_free(boundary, true);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_image_extract_regions(void) {
  fprintf(stderr, "Testing image_extract_regions...");

  struct image8 *im = load_image_fixture("256x256_assorted_polygons.raw");
  struct image32 *labeled;
  while (!(labeled=image_label_regions(im, xmalloc, xrealloc, xfree)));
  struct darray *regions;
  set_allocation_success_chance(0.998);
  while (!(regions=image_extract_regions(im, labeled, xmalloc, xrealloc, xfree)));
  set_allocation_success_chance(0.5);
  assert(regions->len == 4);
  struct region *r1 = darray_index(regions, 0),
                *r2 = darray_index(regions, 1),
                *r3 = darray_index(regions, 2),
                *r4 = darray_index(regions, 3);
  assert(r1->boundary->len == 418);
  assert(r1->area == 7011);
  assert(r1->cx == 142 && r1->cy == 207);
  assert(r2->boundary->len == (256-1)*4);
  assert(r2->area == 256*256-7011-7341-1914);
  assert(r2->cx == 121 && r2->cy == 123);
  assert(r3->boundary->len == 352);
  assert(r3->area == 7341);
  assert(r3->cx == 173 && r3->cy == 96);
  assert(r4->boundary->len == 129);
  assert(r4->area == 1914);
  assert(r4->cx == 60 && r4->cy == 60);
  image8_free(im);
  image32_free(labeled);
  darray_free(regions, true);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

int main(void) {
  void (*(tests[]))(void) = {
    test_image8_new,
    test_image8_usage,
    test_image32_new,
    test_image32_usage,
    test_point_new,
    test_region_new,
    test_region_shallow_free,
    test_region_free,
    test_union_find,
    test_image_label_regions,
    test_image_follow_contour,
    test_image_extract_regions
  };
  int num = sizeof(tests) / sizeof(tests[0]);
  run_tests(num, tests);
}
