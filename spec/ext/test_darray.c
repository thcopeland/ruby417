#include <stdarg.h>
#include "test_helper.h"

static DArray* new_test_array(int length, ...)
{
  DArray* array = darray_new(0);
  va_list argp;

  va_start(argp, length);
  while (length--)
    darray_push(array, (void*) (long) va_arg(argp, int));
  va_end(argp);

  return array;
}

static void assert_darray_eq(DArray* a, DArray* b)
{
  g_assert_cmpint(a->len, ==, b->len);

  for(int i = 0; i < a->len; i++)
    g_assert_cmpint((long) darray_index(a, i), ==, (long) darray_index(b, i));
}

static void assert_darray_vals(DArray* ary, ...)
{
  va_list argp;
  va_start(argp, ary);

  for(int i = 0; i < ary->len; i++)
    g_assert_cmpint((long) darray_index(ary, i), ==, (long) va_arg(argp, int));

  va_end(argp);
}

static void test_darray_new(void)
{
  DArray* ary = darray_new(12);

  g_assert_cmpint(ary->len, ==, 0);
  g_assert_cmpint(ary->capacity, ==, 16); /* next pow of 2 */

  darray_free(ary, NULL);
}

static void test_darray_dup(void)
{
  DArray* ary = new_test_array(3, -1, 42, 7);
  DArray* dup = darray_dup(ary);

  g_assert_cmpint(ary->len, ==, dup->len);
  g_assert_cmpint(dup->capacity, ==, dup->len);
  assert_darray_eq(dup, ary);

  darray_free(ary, NULL);
  darray_free(dup, NULL);
}

static void test_resizing(void)
{
  DArray* ary = darray_new(0);
  g_assert_cmpint(ary->capacity, ==, 2);

  darray_resize_if_necessary(ary, 7);
  g_assert_cmpint(ary->capacity, ==, 8);

  darray_resize_if_necessary(ary, 8);
  g_assert_cmpint(ary->capacity, ==, 8);

  darray_resize_if_necessary(ary, 16);
  g_assert_cmpint(ary->capacity, ==, 32);

  darray_free(ary, NULL);
}

static void test_darray_index(void)
{
  DArray* ary = new_test_array(4, 3, 7, 9, 1);

  g_assert_cmpint((long) darray_index(ary, 0), ==, 3);
  g_assert_cmpint((long) darray_index(ary, 3), ==, 1);
  g_assert_null(darray_index(ary, 4));

  darray_index_set(ary, 0, INT2PTR(0));
  g_assert_cmpint((long) darray_index(ary, 0), ==, 0);

  darray_free(ary, NULL);
}

static void test_darray_push(void)
{
  DArray* ary = darray_new(0);
  g_assert_cmpint(ary->len, ==, 0);
  g_assert_cmpint(ary->capacity, ==, 2);

  darray_push(ary, INT2PTR(16));
  darray_push(ary, INT2PTR(12));
  darray_push(ary, INT2PTR(-12345));

  g_assert_cmpint(ary->len, ==, 3);
  g_assert_cmpint(ary->capacity, ==, 4);
  assert_darray_vals(ary, 16, 12, -12345);

  darray_free(ary, NULL);
}

static void test_darray_remove_fast(void)
{
  DArray* ary = new_test_array(5, 1, 2, 3, 4, 5);

  darray_remove_fast(ary, 1);
  assert_darray_vals(ary, 1, 5, 3, 4);

  darray_remove_fast(ary, 3);
  assert_darray_vals(ary, 1, 5, 3);

  g_assert_cmpint((long) darray_remove_fast(ary, 0), ==, 1);

  darray_free(ary, NULL);
}

static int longcmp(void* a, void* b, void* data)
{
  return ((long) a - (long) b) * (long) data;
}

static void test_darray_msort(void)
{
  DArray* empty = darray_new(0);
  darray_msort(empty, INT2PTR(1), (DArrayCompareFunc) longcmp);
  darray_free(empty, NULL);

  DArray* xs = new_test_array(1, 42);
  darray_msort(xs, INT2PTR(1), (DArrayCompareFunc) longcmp);
  assert_darray_vals(xs, 42);
  darray_free(xs, NULL);

  DArray* s = new_test_array(3, 5, -3, 2);
  darray_msort(s, INT2PTR(1), (DArrayCompareFunc) longcmp);
  assert_darray_vals(s, -3, 2, 5);
  darray_free(s, NULL);

  DArray* m = new_test_array(42, -4731, 3119, 1062, -1723, -3059, 3508, -3571, -4710, 1727, -269, 4645, -3872, -4357, -2464, 3886, 2221, -2342, -2975, 2613, -3085, -3117, 4825, -4767, -2931, 4312, 4223, -3112, 545, -2615, -4283, 308, 2828, 417, -917, -2715, -1226, -319, 3984, -583, -2300, 2487, -959);
  darray_msort(m, INT2PTR(1), (DArrayCompareFunc) longcmp);
  assert_darray_vals(m, -4767, -4731, -4710, -4357, -4283, -3872, -3571, -3117, -3112, -3085, -3059, -2975, -2931, -2715, -2615, -2464, -2342, -2300, -1723, -1226, -959, -917, -583, -319, -269, 308, 417, 545, 1062, 1727, 2221, 2487, 2613, 2828, 3119, 3508, 3886, 3984, 4223, 4312, 4645, 4825);

  darray_msort(m, INT2PTR(-1), (DArrayCompareFunc) longcmp);
  assert_darray_vals(m, 4825, 4645, 4312, 4223, 3984, 3886, 3508, 3119, 2828, 2613, 2487, 2221, 1727, 1062, 545, 417, 308, -269, -319, -583, -917, -959, -1226, -1723, -2300, -2342, -2464, -2615, -2715, -2931, -2975, -3059, -3085, -3112, -3117, -3571, -3872, -4283, -4357, -4710, -4731, -4767);
  darray_free(m, NULL);
}

static void test_darray_insertion_sort(void)
{
  DArray* ary = new_test_array(6, 53, 42, 1, 2, 0, -16);
  DArray* aux = darray_dup(ary);

  darray_insertion_sort(ary, aux, 2, ary->len, INT2PTR(1), (DArrayCompareFunc) longcmp);
  assert_darray_vals(aux, 53, 42, -16, 0, 1, 2);
  assert_darray_vals(ary, 53, 42, 1, 2, 0, -16);

  darray_free(ary, NULL);
  darray_free(aux, NULL);
}

int main(int argc, char** argv) {
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/darray/darray_new", (GTestFunc) test_darray_new);
  g_test_add_func("/darray/darray_dup", (GTestFunc) test_darray_dup);
  g_test_add_func("/darray/resizing", (GTestFunc) test_resizing);
  g_test_add_func("/darray/darray_index", (GTestFunc) test_darray_index);
  g_test_add_func("/darray/darray_push", (GTestFunc) test_darray_push);
  g_test_add_func("/darray/darray_remove_fast", (GTestFunc) test_darray_remove_fast);
  g_test_add_func("/darray/darray_msort", (GTestFunc) test_darray_msort);
  g_test_add_func("/darray/darray_insertion_sort", (GTestFunc) test_darray_insertion_sort);

  return g_test_run();
}
