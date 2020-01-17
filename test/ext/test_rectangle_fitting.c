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
