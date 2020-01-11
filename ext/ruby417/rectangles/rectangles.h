#include <glib.h> /* GArray functions, MAX */

typedef struct {
  gint16 width, height;
  guint8* data;
} RDImage;

typedef struct {
  gint16 width, height;
  gint32* data;
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
  gint16 x, y;
} RDPoint;

typedef struct {
  GArray* boundary;
  gint32 cx, cy;
  gint32 area;
} RDRegion;

typedef struct {
  gint16 cx, cy, width, height;
  double orientation;
} RDRectangle;

static void uf_union(GArray*, gint32, gint32);
static gint32 uf_find(GArray*, gint32);

static RDMatrix* rd_matrix_new(gint16, gint16);
static RDImage* rd_image_new(guint8*, gint16, gint16);

static RDMatrix* rd_label_image_regions(RDImage*);
static gint32 rd_determine_label(RDImage*, RDMatrix*, GArray*, gint16, gint16);

static RDRegion* rd_region_new(void);
static void rd_region_free(RDRegion*);

static GPtrArray* rd_extract_regions(RDImage*, guint8);

static GArray* rd_convex_hull(GArray*);
static int rd_graham_cmp(RDPoint*, RDPoint*, RDPoint*);
static gint32 rd_vector_dot(RDPoint*, RDPoint*, RDPoint*, RDPoint*);
static gint32 rd_vector_cross(RDPoint*, RDPoint*, RDPoint*, RDPoint*);

static RDRectangle* rd_fit_rectangle(GArray*);
static RDPoint* rd_hull_wrap_index(GArray*, gint);
static double rd_line_distance(RDPoint*, RDPoint*, double);
static void rd_determine_fourth_point(RDPoint*, RDPoint*, RDPoint*, RDPoint*);


#ifndef NO_RUBY

#include <ruby.h>

static VALUE detect_rectangles_wrapper(VALUE, VALUE, VALUE, VALUE, VALUE, VALUE);
static VALUE detect_rectangles(guint8*, gint16, gint16, gint32, guint8);
void Init_rectangle_detection(void);

#endif
