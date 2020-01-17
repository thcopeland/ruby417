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
