#include "spec_helper.h"

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

void assert_rectangle(struct rectangle rect, int cx, int cy, long fill, int width, int height, double orientation) {
  assert(rect.width == width && rect.height == height && rect.fill == fill && rect.cx == cx && rect.cy == cy && fabs(rect.orientation-orientation) < 0.0001);
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
  hull_minimal_rectangle(hull, region->area, &rect);
  assert_rectangle(rect, 140, 205, 7011, 51, 161, 1.5708);

  region = darray_index(regions, 1);
  do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
  hull_minimal_rectangle(hull, region->area, &rect);
  assert_rectangle(rect, 127, 128, 49270, 256, 256, 0.0);

  region = darray_index(regions, 2);
  do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
  hull_minimal_rectangle(hull, region->area, &rect);
  assert_rectangle(rect, 151, 89, 7341, 118, 122, 2.97644);

  region = darray_index(regions, 3);
  do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
  hull_minimal_rectangle(hull, region->area, &rect);
  assert_rectangle(rect, 60, 60, 1914, 31, 61, 0.78540);

  image8_free(im);
  image32_free(labeled);
  darray_free(regions, true);
  darray_free(hull, false);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_pair_aligned_rectangles(void) {
  fprintf(stderr, "Testing pair_aligned_rectangles...");

  struct image8 *im = load_image_fixture("256x256_assorted_rectangles.raw");
  struct image32 *labeled;
  while (!(labeled=image_label_regions(im, xmalloc, xrealloc, xfree)));
  struct darray *regions;
  set_allocation_success_chance(0.998);
  while (!(regions=image_extract_regions(im, labeled, xmalloc, xrealloc, xfree)));
  set_allocation_success_chance(0.5);
  assert(regions->len == 13);
  struct darray *rects, *hull, *pairs;
  while (!(rects=darray_new(0, xfree, xmalloc, xrealloc, xfree)));
  while (!(hull=darray_new(0, NULL, xmalloc, xrealloc, xfree)));
  while (!(pairs=darray_new(0, xfree, xmalloc, xrealloc, xfree)));
  for (unsigned i = 0; i < regions->len; i++) {
    struct region *region = darray_index(regions, i);
    do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
    struct rectangle *rect;
    while (!(rect=xmalloc(sizeof(*rect))));
    hull_minimal_rectangle(hull, region->area, rect);
    while (!darray_push(rects, rect));
  }
  struct pairing_settings settings = {
    .area_threshold = 100,
    .rectangularity_threshold = 0.8,
    .angle_variation_threshold = 0.314,
    .area_variation_threshold = 0.5,
    .width_variation_threshold = 0.4,
    .height_variation_threshold = 0.3,
    .guard_aspect_min = 3,
    .guard_aspect_max = 50,
    .barcode_aspect_min = 0,
    .barcode_aspect_max = 10
  };
  assert(rects->len == 13);
  struct rectangle *rect1 = darray_index(rects, 0), *rect2 = darray_index(rects, 1),
                   *rect3 = darray_index(rects, 2), *rect4 = darray_index(rects, 3),
                   *rect5 = darray_index(rects, 4), *rect6 = darray_index(rects, 5),
                   *rect7 = darray_index(rects, 6), *rect8 = darray_index(rects, 7),
                   *rect9 = darray_index(rects, 8), *rect10 = darray_index(rects, 9),
                   *rect11 = darray_index(rects, 10), *rect12 = darray_index(rects, 11),
                   *rect13 = darray_index(rects, 12);
  assert(!rect_qualifies(&settings, rect1));
  assert(!rect_qualifies(&settings, rect2));
  assert(!rect_qualifies(&settings, rect3));
  assert(rect_qualifies(&settings, rect4));
  assert(!rect_qualifies(&settings, rect5));
  assert(rect_qualifies(&settings, rect6));
  assert(rect_qualifies(&settings, rect7));
  assert(rect_qualifies(&settings, rect8));
  assert(!rect_qualifies(&settings, rect9));
  assert(rect_qualifies(&settings, rect10));
  assert(rect_qualifies(&settings, rect11));
  assert(!rect_qualifies(&settings, rect12));
  assert(rect_qualifies(&settings, rect13));
  while (!pair_aligned_rectangles(&settings, rects, pairs)) {
    for (unsigned i = 0; i < pairs->len; i++) xfree(darray_index(pairs, i));
    pairs->len = 0;
  }
  assert(pairs->len == 2);
  struct rectangle_pair *pair1 = darray_index(pairs, 0),
                        *pair2 = darray_index(pairs, 1);
  assert(pair1->score > pair2->score);
  assert(pair1->one->cx == 29 && pair1->one->cy == 66);
  assert(pair1->two->cx == 152 && pair1->two->cy == 76);
  assert(pair2->one->cx == 86 && pair2->one->cy == 240);
  assert(pair2->two->cx == 61 && pair2->two->cy == 177);

  image8_free(im);
  image32_free(labeled);
  darray_free(pairs, true);
  darray_free(rects, true);
  darray_free(regions, true);
  darray_free(hull, false);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

void test_determine_barcode_corners(void) {
  fprintf(stderr, "Testing determine_barcode_corners...");

  struct image8 *im = load_image_fixture("256x256_assorted_rectangles.raw");
  struct image32 *labeled;
  while (!(labeled=image_label_regions(im, xmalloc, xrealloc, xfree)));
  struct darray *regions;
  set_allocation_success_chance(0.998);
  while (!(regions=image_extract_regions(im, labeled, xmalloc, xrealloc, xfree)));
  set_allocation_success_chance(0.5);
  assert(regions->len == 13);
  struct darray *rects, *hull, *pairs;
  while (!(rects=darray_new(0, xfree, xmalloc, xrealloc, xfree)));
  while (!(hull=darray_new(0, NULL, xmalloc, xrealloc, xfree)));
  while (!(pairs=darray_new(0, xfree, xmalloc, xrealloc, xfree)));
  for (unsigned i = 0; i < regions->len; i++) {
    struct region *region = darray_index(regions, i);
    do { hull->len = 0; } while (!boundary_convex_hull(region->boundary, hull));
    struct rectangle *rect;
    while (!(rect=xmalloc(sizeof(*rect))));
    hull_minimal_rectangle(hull, region->area, rect);
    while (!darray_push(rects, rect));
  }
  struct pairing_settings settings = {
    .area_threshold = 100,
    .rectangularity_threshold = 0.8,
    .angle_variation_threshold = 0.314,
    .area_variation_threshold = 0.5,
    .width_variation_threshold = 0.4,
    .height_variation_threshold = 0.3,
    .guard_aspect_min = 3,
    .guard_aspect_max = 50,
    .barcode_aspect_min = 0,
    .barcode_aspect_max = 10
  };
  while (!pair_aligned_rectangles(&settings, rects, pairs)) {
    for (unsigned i = 0; i < pairs->len; i++) xfree(darray_index(pairs, i));
    pairs->len = 0;
  }
  struct rectangle_pair *pair1 = darray_index(pairs, 0),
                        *pair2 = darray_index(pairs, 1);
  struct barcode_corners corners;
  determine_barcode_corners(pair1, &corners);
  assert(corners.upper_left.x == 21 && corners.upper_left.y == 19);
  assert(corners.lower_left.x == 15 && corners.lower_left.y == 111);
  assert(corners.lower_right.x == 164 && corners.lower_right.y == 124);
  assert(corners.upper_right.x == 166 && corners.upper_right.y == 28);

  determine_barcode_corners(pair2, &corners);
  assert(corners.upper_left.x == 99 && corners.upper_left.y == 148);
  assert(corners.lower_left.x == 15 && corners.lower_left.y == 190);
  assert(corners.lower_right.x == 52 && corners.lower_right.y == 259);
  assert(corners.upper_right.x == 125 && corners.upper_right.y == 240);

  image8_free(im);
  image32_free(labeled);
  darray_free(pairs, true);
  darray_free(rects, true);
  darray_free(regions, true);
  darray_free(hull, false);
  assert_mem_clean();

  fprintf(stderr, "PASS\n");
}

int main(void) {
  void (*(tests[]))(void) = {
    test_boundary_convex_hull,
    test_hull_minimal_rectangle,
    test_pair_aligned_rectangles,
    test_determine_barcode_corners
  };
  int num = sizeof(tests) / sizeof(tests[0]);
  run_tests(num, tests);
}
