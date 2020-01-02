#include <stdlib.h> /* malloc, calloc */
#include <glib.h>
#include <math.h> /* pow */

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
  GArray* contour;
  gint32 cx, cy;
  gint32 area;
} RDRegion;

static void uf_union(GArray*, gint32, gint32);
static gint32 uf_find(GArray*, gint32);

static RDMatrix* rd_matrix_new(gint16, gint16);
static RDImage* rd_image_new(guint8*, gint16, gint16);

static RDMatrix* rd_label_image_regions(RDImage*);
static gint32 rd_determine_label(RDImage*, RDMatrix*, GArray*, gint16, gint16);

static RDRegion* rd_region_new(void);
static void rd_region_free(RDRegion*);

static GPtrArray* rd_extract_regions(RDImage*);
static void rd_extract_contour(GArray*, RDMatrix*, gint16, gint16);

static GArray* rd_simplify_polyline(GArray*, gint16);
static void rd_simplify_polyline_recurse(GArray*, GArray*, gint16, gint16, gint16);
static gint16 rd_distance_to_line_sq(RDPoint*, RDPoint*, RDPoint*);

static void uf_union(GArray* acc, gint32 a, gint32 b)
{
  for (gint32 z = acc->len; z <= MAX(a, b); z++) {
    g_array_append_val(acc, z);
  }

  gint32* tmp = &g_array_index(acc, gint32, uf_find(acc, a));
  *tmp = uf_find(acc, b);
}

static gint32 uf_find(GArray* acc, gint32 site)
{
  if (site < 0 || site >= acc->len) {
    return -1;
  } else if (g_array_index(acc, gint32, site) == site) {
    return site;
  } else {
    gint32* tmp = &g_array_index(acc, gint32, site);
    return (*tmp = uf_find(acc, *tmp));
  }
}

static RDMatrix* rd_matrix_new(gint16 width, gint16 height)
{
  RDMatrix* m = malloc(sizeof(RDMatrix));

  if (m) {
    m->width = width;
    m->height = height;
    m->data = calloc(width*height, sizeof(m->data[0]));

    if(!m->data) {
      free(m);
      return NULL;
    }
  }

  return m;
}

static RDImage* rd_image_new(guint8* data, gint16 width, gint16 height)
{
  RDImage* i = malloc(sizeof(RDImage));

  if (i) {
    i->width = width;
    i->height = height;
    i->data = data;
  }

  return i;
}

static RDMatrix* rd_label_image_regions(RDImage* image)
{
  RDMatrix* labels = rd_matrix_new(image->width, image->height);
  GArray* label_eqvs = g_array_sized_new(FALSE, FALSE, sizeof(gint32), 128);
  if (!labels || !label_eqvs) goto abort;

  gint32 x, y, pixel_label, current_label = 1;
  for (y = 0; y < image->height; y++) {
    for (x = 0; x < image->width; x++) {
      pixel_label = rd_determine_label(image, labels, label_eqvs, x, y);

      if (pixel_label) {
        rd_matrix_set(labels, x, y, pixel_label);
      } else {
        rd_matrix_set(labels, x, y, current_label);
        uf_union(label_eqvs, current_label, current_label);
        current_label++;
      }
    }
  }

  for (gint32 z = 0; z < image->width*image->height; z++) {
    labels->data[z] = uf_find(label_eqvs, labels->data[z]);
  }

abort:
  g_array_free(label_eqvs, TRUE);

  return labels;
}

static gint32 rd_determine_label(RDImage* image, RDMatrix* labels, GArray* eqvs, gint16 x, gint16 y)
{
  static const int offsets[2][2] = {{-1, 0}, {0, -1}}; /* 4-connectivity */

  guint8 color = rd_matrix_read_fast(image, x, y);
  gint32 nx, ny, neighbor_label, best_label = 0;

  for (int i = 0; i < 2; i++) {
    nx = offsets[i][0] + x;
    ny = offsets[i][1] + y;

    if (color == rd_matrix_read_safe(image, nx, ny, ~color)) {
      neighbor_label = rd_matrix_read_fast(labels, nx, ny);

      if (best_label) uf_union(eqvs, best_label, neighbor_label);
      if (!best_label || neighbor_label < best_label) best_label = neighbor_label;
    }
  }

  return best_label;
}

static RDRegion* rd_region_new(void)
{
  RDRegion* region = malloc(sizeof(RDRegion));

  if (region) {
    region->contour = g_array_new(FALSE, FALSE, sizeof(RDPoint));
    region->cx = 0;
    region->cy = 0;
    region->area = 0;
  }

  return region;
}

static void rd_region_free(RDRegion* region)
{
  g_array_free(region->contour, TRUE);
  free(region);
}

static GPtrArray* rd_extract_regions(RDImage* image)
{
  GPtrArray* regions = g_ptr_array_new_with_free_func((GDestroyNotify) rd_region_free);
  GHashTable* region_lookup = g_hash_table_new(g_direct_hash, g_direct_equal);
  RDMatrix* labels = rd_label_image_regions(image);
  if (!labels) goto abort;

  gint16 x, y;
  gpointer lookup_key;
  RDRegion* region;

  for (y = 0; y < image->height; y++) {
    for (x = 0; x < image->width; x++) {
      lookup_key = GINT_TO_POINTER(rd_matrix_read_fast(labels, x, y));
      region = g_hash_table_lookup(region_lookup, lookup_key);

      if (!region) {
        region = rd_region_new();
        if (!region) goto abort;

        rd_extract_contour(region->contour, labels, x, y);

        g_hash_table_insert(region_lookup, lookup_key, region);
        g_ptr_array_add(regions, region);
      }

      region->area++;
      region->cx += x;
      region->cy += y;
    }
  }

abort:

  rd_matrix_free(labels);
  g_hash_table_destroy(region_lookup);

  return regions;
}

static void rd_extract_contour(GArray* contour, RDMatrix* labels, gint16 start_x, gint16 start_y)
{
  static const gint RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3;

  RDPoint point = {~start_x, ~start_y};
  gint32 label, target_label = rd_matrix_read_fast(labels, start_x, start_y);
  gint direction = RIGHT;
  gint16 x = start_x, y = start_y;

  do
    {
      label = rd_matrix_read_safe(labels, x, y, ~target_label);

      if (label == target_label) {
        if (x != point.x || y != point.y) {
          point.x = x;
          point.y = y;

          g_array_append_val(contour, point);
        }
        direction = (direction - 1) & 3; /* left turn */
      } else {
         direction = (direction + 1) & 3; /* right turn */
      }

      if      (direction == RIGHT) x++;
      else if (direction == DOWN)  y++;
      else if (direction == LEFT)  x--;
      else if (direction == UP)    y--;
    }
  while (x != start_x || y != start_y);
}

static GArray* rd_simplify_polyline(GArray* polyline, gint16 epsilon)
{
  GArray *simplified = g_array_new(FALSE, FALSE, sizeof(RDPoint));

  if (polyline->len >= 2) {
    g_array_append_val(simplified, g_array_index(polyline, RDPoint, 0));

    rd_simplify_polyline_recurse(polyline, simplified, 0, polyline->len - 1, pow(epsilon, 2));

    g_array_append_val(simplified, g_array_index(polyline, RDPoint, polyline->len - 1));
  } else {
    g_array_append_vals(simplified, polyline->data, polyline->len);
  }

  return simplified;
}

static void rd_simplify_polyline_recurse(GArray* polyline, GArray* simplified, gint16 start, gint16 end, gint16 epsilon_sq)
{
  if (end - start > 1) {
    RDPoint* p1 = &g_array_index(polyline, RDPoint, start);
    RDPoint* p2 = &g_array_index(polyline, RDPoint, end);
    gint16 best_dist = 0, current_dist;
    gint32 best_index = end, current_index = start;

    while (++current_index < end) {
      current_dist = rd_distance_to_line_sq(p1, p2, &g_array_index(polyline, RDPoint, current_index));

      if(current_dist > best_dist) {
        best_dist = current_dist;
        best_index = current_index;
      }
    }

    if (best_index < end && best_dist > epsilon_sq) {
      rd_simplify_polyline_recurse(polyline, simplified, start, best_index, epsilon_sq);

      g_array_append_val(simplified, g_array_index(polyline, RDPoint, best_index));

      rd_simplify_polyline_recurse(polyline, simplified, best_index, end, epsilon_sq);
    }
  }
}

static gint16 rd_distance_to_line_sq(RDPoint* p1, RDPoint* p2, RDPoint* q)
{
  return pow((p2->y - p1->y)*q->x - (p2->x - p1->x)*q->y + p2->x*p1->y - p2->y*p1->x, 2) /
          (pow(p2->y - p1->y, 2) + pow(p2->x - p1->x, 2));
}
