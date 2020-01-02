void assert_point_xy(RDPoint* p, gint px, gint py)
{
  g_assert_cmpint(p->x, ==, px);
  g_assert_cmpint(p->y, ==, py);
}

static void test_line_distance(void)
{
  RDPoint p1 = {.x = 0,  .y = 0};
  RDPoint p2 = {.x = 10, .y = 10};
  RDPoint p3 = {.x = 3,  .y = 6};

  g_assert_cmpint(rd_distance_to_line_sq(&p1, &p2, &p3), ==, 4);
  g_assert_cmpint(rd_distance_to_line_sq(&p1, &p3, &p2), ==, 20);
}

static void test_polyline_simplification(void)
{
  GArray *polyline = g_array_new(FALSE, FALSE, sizeof(RDPoint));

  RDPoint points[] = {{0, 0}, {2, 4}, {3, 6}, {5, 5}, {7, 4}, {8, 3}, {11,2}};
  g_array_append_vals(polyline, points, 7);

  GArray* simplified = rd_simplify_polyline(polyline, 2);

  g_assert_cmpint(simplified->len, ==, 3);
  assert_point_xy(&g_array_index(simplified, RDPoint, 0), 0, 0);
  assert_point_xy(&g_array_index(simplified, RDPoint, 1), 3, 6);
  assert_point_xy(&g_array_index(simplified, RDPoint, 2), 11, 2);

  g_array_free(polyline, TRUE);
  g_array_free(simplified, TRUE);
}
