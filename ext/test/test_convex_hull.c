static void test_convex_hull(void)
{
  RDImage* image = load_image_fixture("256x256_convex_hull_M.raw");
  GPtrArray* regions = rd_extract_regions(image);

  g_assert_nonnull(regions);
  g_assert_cmpint(regions->len, ==, 2);

  RDRegion* r1 = g_ptr_array_index(regions, 0),
          * r2 = g_ptr_array_index(regions, 1);

  GArray* hull1 = rd_convex_hull(r1->boundary),
        * hull2 = rd_convex_hull(r2->boundary);

  g_assert_cmpint(hull1->len, ==, 4);
  assert_point_xy(&g_array_index(hull1, RDPoint, 0), 0, 0);
  assert_point_xy(&g_array_index(hull1, RDPoint, 1), 255, 0);
  assert_point_xy(&g_array_index(hull1, RDPoint, 2), 255, 255);
  assert_point_xy(&g_array_index(hull1, RDPoint, 3), 0, 255);

  g_assert_cmpint(hull2->len, ==, 5);
  assert_point_xy(&g_array_index(hull2, RDPoint, 0), 15, 20);
  assert_point_xy(&g_array_index(hull2, RDPoint, 1), 216, 20);
  assert_point_xy(&g_array_index(hull2, RDPoint, 2), 245, 128);
  assert_point_xy(&g_array_index(hull2, RDPoint, 3), 216, 236);
  assert_point_xy(&g_array_index(hull2, RDPoint, 4), 15, 236);

  g_array_free(hull1, TRUE);
  g_array_free(hull2, TRUE);
  g_ptr_array_free(regions, TRUE);
  rd_matrix_free(image);

  GArray* too_small = g_array_new(FALSE, FALSE, sizeof(RDPoint));

  RDPoint p = {1, 2}, q = {5, 2};
  g_array_append_val(too_small, p);
  g_array_append_val(too_small, q);

  g_assert_null(rd_convex_hull(too_small));

  g_array_free(too_small, TRUE);
}
