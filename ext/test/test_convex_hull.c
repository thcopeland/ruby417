void assert_point_xy(RDPoint* p, gint px, gint py);

void test_clockwise_calculations(void)
{
  RDPoint p1 = {.x = 14, .y = 16};
  RDPoint p2 = {.x = 18, .y = 25};
  RDPoint p3 = {.x = 30, .y = 12};
  RDPoint p4 = {.x = 22, .y = 34};

  g_assert_false(rd_hull_clockwise_turn(&p1, &p2, &p3)); /* counter-clockwise */
  g_assert_false(rd_hull_clockwise_turn(&p1, &p2, &p4)); /* collinear */
  g_assert_true(rd_hull_clockwise_turn(&p3, &p2, &p1));  /* clockwise */
}

void test_convex_hull(void)
{
  GArray* polygon = g_array_new(FALSE, FALSE, sizeof(RDPoint));
  RDPoint points[] = {{0,0}, {2,1}, {5,1}, {6,2}, {7,3}, {3,5}, {2,3}, {0,4}};
  g_array_append_vals(polygon, points, 8);

  GArray* hull = rd_convex_hull(polygon);

  g_assert_cmpint(hull->len, ==, 5);
  assert_point_xy(&g_array_index(hull, RDPoint, 0), 0, 0);
  assert_point_xy(&g_array_index(hull, RDPoint, 1), 5, 1);
  assert_point_xy(&g_array_index(hull, RDPoint, 2), 7, 3);
  assert_point_xy(&g_array_index(hull, RDPoint, 3), 3, 5);
  assert_point_xy(&g_array_index(hull, RDPoint, 4), 0, 4);

  g_array_free(hull, TRUE);
  g_array_free(polygon, TRUE);
}
