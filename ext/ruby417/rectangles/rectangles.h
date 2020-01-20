#include <stdint.h>
#include <stdlib.h>
#include <math.h> /* sin, cos, atan, pow, sqrt, M_PI, M_PI_2 */
#include "../darray/darray.h"

typedef struct {
  uint16_t width, height;
  uint8_t* data;
} RDImage;

typedef struct {
  uint16_t width, height;
  int32_t* data;
} RDMatrix;

#define rd_matrix_read_fast(m, x, y) (m)->data[(x) + (y)*(m)->width]

#define rd_matrix_read_safe(m, x, y, fallback)                                  \
  (((x) >= 0 && (y) >= 0 && (x) < (m)->width && (y) < (m)->height) ?            \
      rd_matrix_read_fast(m, x, y)                                              \
    :                                                                           \
      (fallback))

#define rd_matrix_set(m, x, y, val) do {                                        \
  if ((x) >= 0 && (y) >= 0 && (x) < (m)->width && (y) < (m)->height)            \
    (m)->data[(x)+(y)*(m)->width] = (val);                                      \
  } while(0)

#define rd_matrix_free(m) do {                                                  \
  if(m) {                                                                       \
    free(m->data);                                                              \
    free(m);                                                                    \
  }                                                                             \
} while(0)

typedef struct {
  uint16_t x, y;
} RDPoint;

typedef struct {
  DArray* boundary;
  uint64_t cx, cy;
  uint32_t area;
} RDRegion;

typedef struct {
  uint16_t cx, cy, width, height;
  double orientation;
} RDRectangle;

#define INT2PTR(int) ((void*) (long) (int))
#define PTR2INT(ptr) ((long) (ptr))

#define ALLWELL 0
#define RDNOMEM 12
static unsigned int rd_error = ALLWELL;

static int uf_union(DArray*, uint32_t, uint32_t);
static int64_t uf_find(DArray*, uint32_t);

static RDMatrix* rd_matrix_new(uint16_t, uint16_t);
static RDImage* rd_image_new(uint8_t*, uint16_t, uint16_t);

static RDMatrix* rd_label_image_regions(RDImage*);
static uint32_t rd_determine_label(RDImage*, RDMatrix*, DArray*, uint16_t, uint16_t);

static RDRegion* rd_region_new(void);
static void rd_region_free(RDRegion*);
static RDPoint* rd_point_new(uint16_t, uint16_t);

static DArray* rd_extract_regions(RDImage*, uint8_t);
static void rd_extract_contour(DArray*, RDMatrix*, uint16_t, uint16_t);

static DArray* rd_convex_hull(DArray*);
static int rd_graham_cmp(RDPoint*, RDPoint*, RDPoint*);
static int32_t rd_vector_dot(RDPoint*, RDPoint*, RDPoint*, RDPoint*);
static int32_t rd_vector_cross(RDPoint*, RDPoint*, RDPoint*, RDPoint*);

static RDRectangle* rd_fit_rectangle(DArray*);
static RDPoint* rd_hull_wrap_index(DArray*, uint32_t);
static double rd_line_distance(RDPoint*, RDPoint*, double);
static void rd_determine_fourth_point(RDPoint*, RDPoint*, RDPoint*, RDPoint*);


#ifndef NO_RUBY

#include <ruby.h>

static VALUE detect_rectangles_wrapper(VALUE, VALUE, VALUE, VALUE, VALUE, VALUE);
static VALUE detect_rectangles(uint8_t*, uint16_t, uint16_t, uint32_t, uint8_t);
void Init_rectangle_detection(void);

#endif
