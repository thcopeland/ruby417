#define assert_connected(acc, a, b) \
  g_assert_cmpint(uf_find(acc, a), ==, uf_find(acc, b));

#define assert_not_connected(acc, a, b) \
    g_assert_cmpint(uf_find(acc, a), !=, uf_find(acc, b));

static void test_uf_resizing(void){
  DArray *acc = darray_new(0);
  g_assert_cmpint(acc->len, ==, 0);

  uf_union(acc, 0, 42);
  g_assert_cmpint(acc->len, ==, 43); /* counting zero! */

  darray_free(acc, NULL);
}

static void test_union_find(void){
  DArray *acc = darray_new(0);

  uf_union(acc, 3, 4);
  assert_connected(acc, 3, 4);

  uf_union(acc, 5, 17);
  assert_connected(acc, 5, 17);
  assert_not_connected(acc, 17, 3);

  uf_union(acc, 3, 17);
  assert_connected(acc, 5, 4);
  assert_connected(acc, 3, 17);

  darray_free(acc, NULL);
}
