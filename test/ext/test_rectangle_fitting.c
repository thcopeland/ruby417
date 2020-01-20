#include "test_helper.h"

static void test_convex_hull(void)
{
  RDImage* image = load_image_fixture("256x256_convex_hull_M.raw");
  DArray* regions = rd_extract_regions(image, 0);

  g_assert_nonnull(regions);
  g_assert_cmpint(regions->len, ==, 2);

  RDRegion* r1 = darray_index(regions, 0),
          * r2 = darray_index(regions, 1);

  DArray* hull1 = rd_convex_hull(r1->boundary),
        * hull2 = rd_convex_hull(r2->boundary);

  g_assert_cmpint(hull1->len, ==, 5);
  assert_point_xy(darray_index(hull1, 0), 15, 20);
  assert_point_xy(darray_index(hull1, 1), 216, 20);
  assert_point_xy(darray_index(hull1, 2), 245, 128);
  assert_point_xy(darray_index(hull1, 3), 216, 236);
  assert_point_xy(darray_index(hull1, 4), 15, 236);

  g_assert_cmpint(hull2->len, ==, 4);
  assert_point_xy(darray_index(hull2, 0), 0, 0);
  assert_point_xy(darray_index(hull2, 1), 255, 0);
  assert_point_xy(darray_index(hull2, 2), 255, 255);
  assert_point_xy(darray_index(hull2, 3), 0, 255);

  darray_free(hull1, NULL);
  darray_free(hull2, NULL);
  darray_free(regions, (DArrayFreeFunc) rd_region_free);
  rd_matrix_free(image);

  DArray* too_small = darray_new(2);

  RDPoint p = {1, 2}, q = {5, 2};
  darray_push(too_small, &p);
  darray_push(too_small, &q);

  g_assert_null(rd_convex_hull(too_small));

  darray_free(too_small, NULL);
}

static void assert_rectangle(RDRectangle* rect, int x, int y, int w, int h, double orientation)
{
  g_assert_cmpint(rect->cx, ==, x);
  g_assert_cmpint(rect->cy, ==, y);
  g_assert_cmpint(rect->width, ==, w);
  g_assert_cmpint(rect->height, ==, h);
  g_assert_true(fabs(rect->orientation - orientation) < M_PI/128);
}

static void test_rectangle_fitting(void)
{
  RDImage* image = load_image_fixture("256x256_assorted_polygons.raw");
  DArray* regions = rd_extract_regions(image, 0);

  g_assert_cmpint(regions->len, ==, 4);

  RDRegion* r1 = darray_index(regions, 0),
          * r2 = darray_index(regions, 1),
          * r3 = darray_index(regions, 2),
          * r4 = darray_index(regions, 3);

  DArray* hull1 = rd_convex_hull(r1->boundary),
        * hull2 = rd_convex_hull(r2->boundary),
        * hull3 = rd_convex_hull(r3->boundary),
        * hull4 = rd_convex_hull(r4->boundary);

  RDRectangle* rect1 = rd_fit_rectangle(hull1),
             * rect2 = rd_fit_rectangle(hull2),
             * rect3 = rd_fit_rectangle(hull3),
             * rect4 = rd_fit_rectangle(hull4);

  assert_rectangle(rect1, 140, 205, 160, 50, 0);
  assert_rectangle(rect2, 127, 127, 255, 255, 0);
  assert_rectangle(rect3, 184, 106, 147, 95, -0.833);
  assert_rectangle(rect4, 60, 61, 30, 60, M_PI_4);

  free(rect1);
  free(rect2);
  free(rect3);
  free(rect4);

  darray_free(hull1, NULL);
  darray_free(hull2, NULL);
  darray_free(hull3, NULL);
  darray_free(hull4, NULL);

  darray_free(regions, (DArrayFreeFunc) rd_region_free);

  rd_matrix_free(image);
}

static int rectangle_runthrough(RDImage* image)
{
  rd_error = ALLWELL;

  DArray* regions = rd_extract_regions(image, 0);
  if (rd_error != ALLWELL) return 0;

  RDRegion* region = darray_index(regions, 1);
  DArray* hull = rd_convex_hull(region->boundary);

  if (rd_error != ALLWELL) {
    darray_free(regions, (DArrayFreeFunc) rd_region_free);
    return 0;
  }

  RDRectangle* rect = rd_fit_rectangle(hull);

  if (rd_error != ALLWELL) {
    darray_free(regions, (DArrayFreeFunc) rd_region_free);
    darray_free(hull, NULL);
    return 0;
  }

  darray_free(regions, (DArrayFreeFunc) rd_region_free);
  darray_free(hull, NULL);
  free(rect);

  return 1;
}

static void test_allocation_failures(void)
{
  /* Run the entire rectangle detection process several hundred times, failing a
     different memory allocation each time, until it works. If this test doesn't
     segfault, chances are that memory allocation failures are handled properly. */
  RDImage* image = load_image_fixture("16x16_region_labeling_spiral.raw");

  int i = 0;
  do {
    fail_subsequent_alloc(i++);
  } while(rectangle_runthrough(image) == 0);
  fail_subsequent_alloc(-1);

  free(image);
}

int main(int argc, char* argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/convex_hull", (GTestFunc) test_convex_hull);
  g_test_add_func("/rectangles", (GTestFunc) test_rectangle_fitting);
  g_test_add_func("/rectangles/allocations", (GTestFunc) test_allocation_failures);

  return g_test_run();
}
