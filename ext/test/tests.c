#include "../ruby417/ruby417.c"
#include "test_union_find.c"
#include "test_region_labeling.c"
#include "test_contour_extraction.c"

int main(int argc, char *argv[]) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/union-find/resizing", (GTestFunc) test_uf_resizing);
  g_test_add_func("/union-find/usage", (GTestFunc) test_union_find);
  g_test_add_func("/region labeling/image", (GTestFunc) test_region_labeling);
  g_test_add_func("/contour extraction/image", (GTestFunc) test_contour_extraction);
  g_test_add_func("/contour extraction/points", (GTestFunc) test_contour_points);
  return g_test_run();
}
