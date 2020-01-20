#include "test_helper.h"

#define assert_matrix_eq(m1, m2) do {                                           \
  g_assert_cmpint(m1->height, ==, m2->height);                                  \
  g_assert_cmpint(m1->width, ==, m2->width);                                    \
                                                                                \
  for (int i = 0; i < (m1->width)*(m2->height); i++)                            \
    g_assert_cmpint((int) m1->data[i], ==, (int) m2->data[i]);                  \
} while(0);

static void test_region_labeling(void) {
  RDImage *image = load_image_fixture("16x16_region_labeling_spiral.raw");
  RDMatrix *regions = load_matrix_fixture("16x16_region_labeling_spiral_labeled");

  RDMatrix *labeled = rd_label_image_regions(image);

  assert_matrix_eq(regions, labeled);

  rd_matrix_free(labeled);
  rd_matrix_free(regions);
  rd_matrix_free(image);
}

static void test_region_extraction(void)
{
  RDImage* image = load_image_fixture("320x320_solitary_circle.raw");
  DArray* regions = rd_extract_regions(image, 0);

  g_assert_nonnull(regions);
  g_assert_cmpint(regions->len, ==, 2);

  RDRegion* r1 = darray_index(regions, 0),
          * r2 = darray_index(regions, 1);

  g_assert_cmpint(r1->boundary->len, ==, 716);
  g_assert_cmpint(r2->boundary->len, ==, 1276); /* (320-1)*4 */
  g_assert_cmpint(r1->area, ==, 27325);
  g_assert_cmpint(r2->area, ==, 75075); /* 320**2 - 27325 */
  g_assert_cmpint(r2->cx, ==, 158);
  g_assert_cmpint(r2->cy, ==, 158);
  g_assert_cmpint(r1->cx, ==, 162);
  g_assert_cmpint(r1->cy, ==, 162);

  assert_point_xy(darray_index(r2->boundary, 0), 0, 0);
  assert_point_xy(darray_index(r2->boundary, 20), 20, 0);
  assert_point_xy(darray_index(r2->boundary, 1275), 0, 1);

  assert_point_xy(darray_index(r1->boundary, 0), 140, 70);
  assert_point_xy(darray_index(r1->boundary, 715), 140, 71);

  rd_matrix_free(image);
  darray_free(regions, (DArrayFreeFunc) rd_region_free);
}

static void test_region_color_threshold(void)
{
  RDImage* image = load_image_fixture("320x320_solitary_circle.raw");

  DArray* regions = rd_extract_regions(image, 0);
  g_assert_cmpint(regions->len, ==, 2);
  darray_free(regions, (DArrayFreeFunc) rd_region_free);

  regions = rd_extract_regions(image, 15);
  g_assert_cmpint(regions->len, ==, 1);
  darray_free(regions, (DArrayFreeFunc) rd_region_free);

  regions = rd_extract_regions(image, 255);
  g_assert_cmpint(regions->len, ==, 1);
  darray_free(regions, (DArrayFreeFunc) rd_region_free);

  rd_matrix_free(image);
}

int main(int argc, char* argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/regions/labeling", (GTestFunc) test_region_labeling);
  g_test_add_func("/regions/extraction", (GTestFunc) test_region_extraction);
  g_test_add_func("/regions/threshold", (GTestFunc) test_region_color_threshold);

  return g_test_run();
}
