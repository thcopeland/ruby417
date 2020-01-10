#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

static int fail_nth_allocation = -1;

#define malloc(size) (fail_nth_allocation < 0 || (--fail_nth_allocation) ? malloc(size) : (fail_nth_allocation = -1, NULL))

#include "../ruby417/ruby417.c"

#define FIXTURES_DIR "fixtures"

#define assert_matrix_eq(m1, m2) do {                                           \
  g_assert_cmpint(m1->height, ==, m2->height);                                  \
  g_assert_cmpint(m1->width, ==, m2->width);                                    \
                                                                                \
  for (int i = 0; i < (m1->width)*(m2->height); i++)                            \
    g_assert_cmpint((int) m1->data[i], ==, (int) m2->data[i]);                  \
} while(0);

char* load_fixture_data(char* filename, int num);
RDImage* load_image_fixture(char* filename);
RDMatrix* load_matrix_fixture(char* filename);

void assert_point_xy(RDPoint* p, int x, int y);
