static void test_region_extraction(void)
{
  RDImage* image = load_image_fixture("320x320_solitary_circle.raw");
  GPtrArray* regions = rd_extract_regions(image);

  g_assert_nonnull(regions);
  g_assert_cmpint(regions->len, ==, 2);

  RDRegion* r1 = g_ptr_array_index(regions, 0),
          * r2 = g_ptr_array_index(regions, 1);

  g_assert_cmpint(r2->boundary->len, ==, 696); /* "experimentally determined" */
  g_assert_cmpint(r1->boundary->len, ==, 1976); /* (320-1)*4 + 696 + 4 */
  g_assert_cmpint(r2->area, ==, 27325); /* also exp. det. */
  g_assert_cmpint(r1->area, ==, 75075); /* 320**2 - 27325 */
  g_assert_cmpint(r1->cx, ==, 158); /* 320 / 2 - 2 (zero-based indexing and rounding down) */
  g_assert_cmpint(r1->cy, ==, 158);
  g_assert_cmpint(r2->cx, ==, 162); /* exp. det. */
  g_assert_cmpint(r2->cy, ==, 162);

  assert_point_xy(&g_array_index(r1->boundary, RDPoint, 0), 0, 0);
  assert_point_xy(&g_array_index(r1->boundary, RDPoint, 20), 20, 0);
  assert_point_xy(&g_array_index(r1->boundary, RDPoint, 341), 319, 11);
  assert_point_xy(&g_array_index(r1->boundary, RDPoint, 1975), 319, 319);

  assert_point_xy(&g_array_index(r2->boundary, RDPoint, 0), 140, 70);
  assert_point_xy(&g_array_index(r2->boundary, RDPoint, 695), 184, 254);

  rd_matrix_free(image);
  g_ptr_array_free(regions, TRUE);
}
