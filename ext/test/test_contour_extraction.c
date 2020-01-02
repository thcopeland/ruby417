void assert_point_xy(RDPoint* p, gint px, gint py);

static guint8 test_ce_image[] = {
  0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 0,
  0, 1, 1, 1, 1, 0,
  0, 0, 0, 1, 1, 0,
  0, 0, 0, 1, 1, 0,
  0, 1, 1, 1, 1, 0,
  0, 1, 1, 1, 1, 0,
  0, 0, 0, 0, 0, 0
};

static guint8 test_ce_image_simple[] = {
  1, 1, 1,
  1, 1, 1,
  1, 1, 1
};

void test_contour_extraction(void)
{
  RDImage* image = rd_image_new(test_ce_image, 6, 8);
  GPtrArray* regions = rd_extract_regions(image);

  g_assert_cmpint(regions->len, ==, 2);
  g_assert_cmpint(((RDRegion*) g_ptr_array_index(regions, 0))->area, ==, 28);
  g_assert_cmpint(((RDRegion*) g_ptr_array_index(regions, 1))->area, ==, 20);

  g_ptr_array_free(regions, TRUE);
  free(image);
}

void test_contour_points(void)
{
  RDImage* image = rd_image_new(test_ce_image_simple, 3, 3);
  GPtrArray* regions = rd_extract_regions(image);
  RDRegion* region = g_ptr_array_index(regions, 0);

  g_assert_cmpint(regions->len, ==, 1);
  g_assert_cmpint(region->area, ==, 9);
  g_assert_cmpint(region->contour->len, ==, 8);

  assert_point_xy(&g_array_index(region->contour, RDPoint, 0), 0, 0);
  assert_point_xy(&g_array_index(region->contour, RDPoint, 1), 1, 0);
  assert_point_xy(&g_array_index(region->contour, RDPoint, 2), 2, 0);
  assert_point_xy(&g_array_index(region->contour, RDPoint, 3), 2, 1);
  assert_point_xy(&g_array_index(region->contour, RDPoint, 4), 2, 2);
  assert_point_xy(&g_array_index(region->contour, RDPoint, 5), 1, 2);
  assert_point_xy(&g_array_index(region->contour, RDPoint, 6), 0, 2);
  assert_point_xy(&g_array_index(region->contour, RDPoint, 7), 0, 1);

  g_ptr_array_free(regions, TRUE);
  free(image);
}
