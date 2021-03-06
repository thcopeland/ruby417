#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/* These macros wrap their stdlib counterparts, except that they produce NULL
   as directed by fail_subsequent_allocation, simulating allocation failure.  */
static long fail_subsequent_allocation = -1;
#define fail_subsequent_alloc(n) (fail_subsequent_allocation = n)
#define malloc(s) ((fail_subsequent_allocation < 0 || fail_subsequent_allocation-- > 0)?malloc(s):NULL)
#define calloc(n, s) ((fail_subsequent_allocation < 0 || fail_subsequent_allocation-- > 0)?calloc(n, s):NULL)
#define realloc(p, s) ((fail_subsequent_allocation < 0 || fail_subsequent_allocation-- > 0)?realloc(p, s):NULL)

#include "rectangles/rectangles.c"
#include "darray/darray.h"

#define FIXTURES_DIR "fixtures"

char* load_fixture_data(char* filename, int num)
{
  char* data = malloc(num);

  char* full_path = malloc(strlen(FIXTURES_DIR) + strlen(filename) + 2); // account for slash and nul
  sprintf(full_path, "%s/%s", FIXTURES_DIR, filename);

  FILE* f = fopen(full_path, "rb");

  if(!f) {
    perror("ERR");
    exit(-1);
  }

  // TODO multibyte
  size_t bytes_read = fread(data, 1, num, f);

  if(bytes_read != num) {
    fprintf(stderr, "ERR: unexpected EOF while reading %s (got %li, expected %i)\n", full_path, bytes_read, num);
    exit(-1);
  }

  fclose(f);
  free(full_path);

  return data;
}

RDImage* load_image_fixture(char* filename)
{
  RDImage* img = malloc(sizeof(RDImage));
  unsigned int width, height;

  sscanf(filename, "%ux%u", &width, &height);

  img->width = width;
  img->height = height;
  img->data = (guint8*) load_fixture_data(filename, width*height);

  return img;
}

RDMatrix* load_matrix_fixture(char* filename)
{
  RDMatrix* mtx = malloc(sizeof(RDImage));
  unsigned int width, height;

  sscanf(filename, "%ux%u", &width, &height);
  char* data = load_fixture_data(filename, width*height);

  mtx->width = width;
  mtx->height = height;
  mtx->data = malloc(width*height*sizeof(int32_t));

  for(int i = 0; i < width*height; i++)
    mtx->data[i] = (int32_t) data[i];

  free(data);

  return mtx;
}

void assert_point_xy(RDPoint* p, int x, int y)
{
  g_assert_cmpint(p->x, ==, x);
  g_assert_cmpint(p->y, ==, y);
}
