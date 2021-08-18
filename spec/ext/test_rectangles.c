#include "spec_helper.h"

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

void test_boundary_convex_hull(void) {
  fprintf(stderr, "Testing boundary_convex_hull...");

  struct image8 *im = load_image_fixture("256x256_convex_hull_M.raw");
  struct image32 *labeled;
  while (!(labeled=image_label_regions(im, xmalloc, xrealloc, xfree)));

  struct darray *regions;
  set_allocation_success_chance(0.998);
  while (!(regions=image_extract_regions(im, labeled, xmalloc, xrealloc, xfree)));
  struct region *region = darray_index(regions, 0);
  assert(regions->len == 2);
  assert(region->area == 23932);
  assert(region->boundary->len == 1135);
  struct darray *hull;
  set_allocation_success_chance(0.8);
  while (!(hull=darray_new(0, NULL, xmalloc, xrealloc, xfree)));
  while (!boundary_convex_hull(region->boundary, hull)) hull->len = 0;
  set_allocation_success_chance(0.5);
  assert(hull->len == 5);
  struct point *p1 = darray_index(hull, 0),
               *p2 = darray_index(hull, 1),
               *p3 = darray_index(hull, 2),
               *p4 = darray_index(hull, 3),
               *p5 = darray_index(hull, 4);
  assert(p1->x == 15 && p1->y == 20);
  assert(p2->x == 216 && p2->y == 20);
  assert(p3->x == 245 && p3->y == 128);
  assert(p4->x == 216 && p4->y == 236);
  assert(p5->x == 16 && p5->y == 236);
  image8_free(im);
  image32_free(labeled);
  darray_free(regions, true);
  darray_free(hull, false);
  assert_mem_clean();

  // with a small input
  struct darray *boundary;
  while (!(hull=darray_new(0, NULL, xmalloc, xrealloc, xfree)));
  while (!(boundary=darray_new(0, xfree, xmalloc, xrealloc, xfree)));
  while (!(p1=point_new(1, 2, xmalloc)));
  while (!(p2=point_new(3, 5, xmalloc)));
  while (!(p3=point_new(0, 10, xmalloc)));
  while (!darray_push(boundary, p1));
  while (!darray_push(boundary, p2));
  while (!darray_push(boundary, p3));
  while (!boundary_convex_hull(boundary, hull)) hull->len = 0;
  assert(hull->len == 3);
  darray_free(boundary, true);
  darray_free(hull, false);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void assert_rectangle(struct rectangle rect, int cx, int cy, int width, int height, double orientation) {
  assert(rect.width == width && rect.height == height && rect.cx == cx && rect.cy == cy && fabs(rect.orientation-orientation) < 0.0001);
}

void test_hull_minimal_rectangle(void) {
  fprintf(stderr, "Testing hull_minimal_rectangle...");

  struct image8 *im = load_image_fixture("256x256_assorted_polygons.raw");
  struct image32 *labeled;
  while (!(labeled=image_label_regions(im, xmalloc, xrealloc, xfree)));
  struct darray *regions;
  set_allocation_success_chance(0.998);
  while (!(regions=image_extract_regions(im, labeled, xmalloc, xrealloc, xfree)));
  assert(regions->len == 4);
  struct darray *hull;
  struct region *region;
  struct rectangle rect;
  set_allocation_success_chance(0.9);
  while (!(hull=darray_new(0, NULL, xmalloc, xrealloc, xfree)));

  region = darray_index(regions, 0);
  do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
  hull_minimal_rectangle(hull, &rect);
  assert_rectangle(rect, 140, 205, 161, 51, 0.0);

  region = darray_index(regions, 1);
  do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
  hull_minimal_rectangle(hull, &rect);
  assert_rectangle(rect, 127, 128, 256, 256, 0.0);

  region = darray_index(regions, 2);
  do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
  hull_minimal_rectangle(hull, &rect);
  assert_rectangle(rect, 151, 89, 122, 118, 1.4056);

  region = darray_index(regions, 3);
  do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
  hull_minimal_rectangle(hull, &rect);
  assert_rectangle(rect, 60, 60, 31, 61, 0.78540);

  image8_free(im);
  image32_free(labeled);
  darray_free(regions, true);
  darray_free(hull, false);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

int main(void) {
  void (*(tests[]))(void) = {
    test_point_new,
    test_region_new,
    test_region_shallow_free,
    test_region_free,
    test_union_find,
    test_image_label_regions,
    test_image_follow_contour,
    test_image_extract_regions,
    test_boundary_convex_hull,
    test_hull_minimal_rectangle
  };
  int num = sizeof(tests) / sizeof(tests[0]);
  run_tests(num, tests);
}
