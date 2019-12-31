#define assert_point_xy(p, px, py) do {                                         \
  g_assert_cmpint((p)->x, ==, (px));                                            \
  g_assert_cmpint((p)->y, ==, (py));                                            \
} while(0)

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
  GList *regions = rd_extract_region_contours(test_ce_image, 6, 8);
  g_assert_cmpint(g_list_length(regions), ==, 2);
  g_assert_cmpint(((RDRegion*) g_list_nth_data(regions, 0))->area, ==, 20);
  g_assert_cmpint(((RDRegion*) g_list_nth_data(regions, 1))->area, ==, 28);
  g_list_free_full(regions, (GDestroyNotify) rd_region_free);
}

void test_contour_points(void)
{
  GList *regions = rd_extract_region_contours(test_ce_image_simple, 3, 3);
  RDRegion *region = g_list_nth_data(regions, 0);

  g_assert_cmpint(g_list_length(regions), ==, 1);
  g_assert_cmpint(region->area, ==, 9);
  g_assert_cmpint(g_list_length(region->contour), ==, 8);

  assert_point_xy((RDContourPoint*) g_list_nth_data(region->contour, 0), 0, 0);
  assert_point_xy((RDContourPoint*) g_list_nth_data(region->contour, 1), 1, 0);
  assert_point_xy((RDContourPoint*) g_list_nth_data(region->contour, 2), 2, 0);
  assert_point_xy((RDContourPoint*) g_list_nth_data(region->contour, 3), 2, 1);
  assert_point_xy((RDContourPoint*) g_list_nth_data(region->contour, 4), 2, 2);
  assert_point_xy((RDContourPoint*) g_list_nth_data(region->contour, 5), 1, 2);
  assert_point_xy((RDContourPoint*) g_list_nth_data(region->contour, 6), 0, 2);
  assert_point_xy((RDContourPoint*) g_list_nth_data(region->contour, 7), 0, 1);

  g_list_free_full(regions, (GDestroyNotify) rd_region_free);
}
