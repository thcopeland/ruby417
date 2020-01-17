#include "utilities.h"
#include "test_union_find.c"
#include "test_region_labeling.c"
#include "test_region_extraction.c"
#include "test_convex_hull.c"
#include "test_rectangle_fitting.c"

int main(int argc, char* argv[])
{
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/union_find/resizing", (GTestFunc) test_uf_resizing);
  g_test_add_func("/union_find/usage", (GTestFunc) test_union_find);
  g_test_add_func("/labeling/image", (GTestFunc) test_region_labeling);
  g_test_add_func("/labeling/alloc", (GTestFunc) test_region_labeling_allocation);
  g_test_add_func("/regions", (GTestFunc) test_region_extraction);
  g_test_add_func("/regions/threshold", (GTestFunc) test_region_color_threshold);
  g_test_add_func("/convex_hull", (GTestFunc) test_convex_hull);
  g_test_add_func("/rectangles", (GTestFunc) test_rectangle_fitting);

  return g_test_run();
}
