#include <stdint.h>
#include "../darray/darray.h"

typedef struct {
  int16_t width, height;
  uint8_t* data;
} RDImage;

typedef struct {
  int16_t width, height;
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
  int16_t x, y;
} RDPoint;

typedef struct {
  DArray* boundary;
  int32_t cx, cy;
  int32_t area;
} RDRegion;

typedef struct {
  int16_t cx, cy, width, height;
  double orientation;
} RDRectangle;

#define INT2PTR(int) ((void*) (long) (int))
#define PTR2INT(ptr) ((long) (ptr))

static void uf_union(DArray*, int32_t, int32_t);
static int32_t uf_find(DArray*, int32_t);

static RDMatrix* rd_matrix_new(int16_t, int16_t);
static RDImage* rd_image_new(uint8_t*, int16_t, int16_t);

static RDMatrix* rd_label_image_regions(RDImage*);
static int32_t rd_determine_label(RDImage*, RDMatrix*, DArray*, int16_t, int16_t);

static RDRegion* rd_region_new(void);
static void rd_region_free(RDRegion*);
static RDPoint* rd_point_new(int16_t, int16_t);

static DArray* rd_extract_regions(RDImage*, uint8_t);
static void rd_extract_contour(DArray*, RDMatrix*, int16_t, int16_t);

static DArray* rd_convex_hull(DArray*);
static int rd_graham_cmp(RDPoint*, RDPoint*, RDPoint*);
static int32_t rd_vector_dot(RDPoint*, RDPoint*, RDPoint*, RDPoint*);
static int32_t rd_vector_cross(RDPoint*, RDPoint*, RDPoint*, RDPoint*);

static RDRectangle* rd_fit_rectangle(DArray*);
static RDPoint* rd_hull_wrap_index(DArray*, int32_t);
static double rd_line_distance(RDPoint*, RDPoint*, double);
static void rd_determine_fourth_point(RDPoint*, RDPoint*, RDPoint*, RDPoint*);


#ifndef NO_RUBY

#include <ruby.h>

static VALUE detect_rectangles_wrapper(VALUE, VALUE, VALUE, VALUE, VALUE, VALUE);
static VALUE detect_rectangles(uint8_t*, uint16_t, uint16_t, uint32_t, uint8_t);
void Init_rectangle_detection(void);

#endif
