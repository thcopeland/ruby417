#include <glib.h>
#include <stdlib.h>
#include <stdio.h>

#define rd_matrix_read(m, w, h, x, y, fallback)                                 \
  (((x) >= 0 && (y) >= 0 && (x) < (w) && (y) < (h)) ?                           \
    (m)[(x)+(y)*(w)]                                                            \
  :                                                                             \
    (fallback))

#define rd_matrix_set(m, w, h, x, y, val) do {                                  \
  if((x) >= 0 && (y) >= 0 && (x) < (w) && (y) < (h))                            \
    (m)[(x)+(y)*(w)] = (val);                                                   \
} while(0)

typedef struct { gint16 x, y; } RDContourPoint;

typedef struct {
  GList *contour;
  gint32 cx, cy;
  gint32 area;
} RDRegion;

static void uf_union(GArray*, gint32, gint32);
static gint32 uf_find(GArray*, gint32);

static gint32 *rd_label_image_regions(guint8*, gint32, gint32);
static gint32 rd_determine_label(guint8*, gint32*, gint32, gint32, GArray*, gint32, gint32);

static RDContourPoint *rd_point_new(gint16, gint16);
static RDRegion *rd_region_new(void);
static void rd_region_free(RDRegion*);

static GList *rd_extract_region_contours(guint8*, gint16, gint16, gint32);
static GList *rd_extract_contour(gint32*, gint16, gint16, gint16, gint16);
static gboolean rd_filter_by_area(gpointer, gpointer, gpointer);

static void uf_union(GArray *acc, gint32 a, gint32 b){
  for(gint32 z = acc->len; z <= MAX(a, b); z++){
    g_array_append_val(acc, z);
  }

  gint32 *tmp = &g_array_index(acc, gint32, uf_find(acc, a));
  *tmp = uf_find(acc, b);
}

static gint32 uf_find(GArray *acc, gint32 site){
  if(site < 0 || site >= acc->len){
    return -1;
  } else if(g_array_index(acc, gint32, site) == site){
    return site;
  } else {
    gint32 *tmp = &g_array_index(acc, gint32, site);
    return (*tmp = uf_find(acc, *tmp));
  }
}

static gint32 *rd_label_image_regions(guint8 *image, gint32 width, gint32 height){
  gint32 *labels = calloc(width*height, sizeof(gint32));
  GArray *label_eqvs = g_array_new(FALSE, FALSE, sizeof(gint32));
  if(!labels || !label_eqvs) goto abort;

  gint32 x, y, pixel_label, current_label = 1;
  for(y = 0; y < height; y++){
    for(x = 0; x < width; x++){
      pixel_label = rd_determine_label(image, labels, width, height, label_eqvs, x, y);

      if(pixel_label) {
        rd_matrix_set(labels, width, height, x, y, pixel_label);
      } else {
        rd_matrix_set(labels, width, height, x, y, current_label);
        uf_union(label_eqvs, current_label, current_label);
        current_label++;
      }
    }
  }

  for(gint32 z = 0; z < width*height; z++){
    labels[z] = uf_find(label_eqvs, labels[z]);
  }

abort:
  g_array_free(label_eqvs, TRUE);

  return labels;
}

static gint32 rd_determine_label(guint8 *image, gint32 *labels, gint32 width, gint32 height, GArray *eqvs, gint32 x, gint32 y){
  static int offsets[2][2] = {{-1, 0}, {0, -1}}; /* 4-connectivity */

  guint8 color = rd_matrix_read(image, width, height, x, y, 0);
  gint32 nx, ny, neighbor_label, best_label = 0;
  for(int i = 0; i < 2; i++){
    nx = offsets[i][0] + x;
    ny = offsets[i][1] + y;

    if(color == rd_matrix_read(image, width, height, nx, ny, ~color)){
      neighbor_label = rd_matrix_read(labels, width, height, nx, ny, best_label);

      if(best_label) uf_union(eqvs, best_label, neighbor_label);
      if(!best_label || neighbor_label < best_label) best_label = neighbor_label;
    }
  }

  return best_label;
}

static RDContourPoint *rd_point_new(gint16 x, gint16 y)
{
  RDContourPoint *p = malloc(sizeof(RDRegion));

  if (p) {
    p->x = x;
    p->y = y;
  }

  return p;
}

static RDRegion *rd_region_new(void)
{
  RDRegion *region = malloc(sizeof(RDRegion));

  if (region) {
    region->contour = NULL;
    region->cx = 0;
    region->cy = 0;
    region->area = 0;
  }

  return region;
}

static void rd_region_free(RDRegion *region)
{
  g_list_free_full(region->contour, (GDestroyNotify) free);
  free(region);
}

static GList *rd_extract_region_contours(guint8 *image, gint16 width, gint16 height, gint32 area_threshold)
{
  GList *regions = NULL;
  GHashTable *region_lookup = g_hash_table_new(g_direct_hash, g_direct_equal);
  gint32 *labels = rd_label_image_regions(image, width, height);
  if (!labels || !region_lookup) goto abort;

  gint16 x, y;
  gpointer lookup_key;
  RDRegion *region;
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      lookup_key = GINT_TO_POINTER(rd_matrix_read(labels, width, height, x, y, 0));
      region = g_hash_table_lookup(region_lookup, lookup_key);

      if (!region) {
        region = rd_region_new();
        if(!region) goto abort;

        region->contour = rd_extract_contour(labels, width, height, x, y);
        if (!region->contour) goto abort;

        g_hash_table_insert(region_lookup, lookup_key, region);
      }

      region->area++;
      region->cx += x;
      region->cy += y;
    }
  }

  g_hash_table_foreach_remove(region_lookup, (GHRFunc) rd_filter_by_area, &area_threshold);

  regions = g_hash_table_get_values(region_lookup);

abort:

  free(labels);
  g_hash_table_destroy(region_lookup);

  return regions;
}

static GList *rd_extract_contour(gint32 *labels, gint16 width, gint16 height, gint16 start_x, gint16 start_y)
{
  static const gint RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3;

  GList *contour = NULL;
  RDContourPoint *point;
  gint32 label, target_label = rd_matrix_read(labels, width, height, start_x, start_y, 0);
  gint direction = RIGHT;
  gint16 x = start_x, y = start_y;

  do
    {
      label = rd_matrix_read(labels, width, height, x, y, ~target_label);

      if (label == target_label) {
        point = rd_point_new(x, y);
        if (!point) goto nomem;

        contour = g_list_append(contour, point);
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

  return contour;

nomem:

  g_list_free_full(contour, (GDestroyNotify) free);
  return NULL;
}

static gboolean rd_filter_by_area(gpointer label_ptr, gpointer region_ptr, gpointer threshold_ptr)
{
  return ((RDRegion *) region_ptr)->area < *(gint32 *) threshold_ptr;
}


int main(int argc, char **argv) {
  static guint8 image[] = {
    0, 0, 0, 0,
    0, 1, 1, 0,
    1, 1, 1, 1,
    0, 1, 1, 1
  };

  // gint32 *labels = rd_label_image_regions(image, 4, 4);
  // GList *contour = rd_extract_contour(labels, 4, 4, 0, 0);

  // for(GList *r=contour; r; r=r->next){
  //   RDContourPoint *p = (RDContourPoint*) r->data;
  //   printf("%i,%i\n", p->x, p->y);
  // }
  GList *regions = rd_extract_region_contours(image, 4, 4, 4);
  for(GList *r=regions; r; r=r->next){
    RDRegion *rd = ((RDRegion*) r->data);
    printf("Area: %i, Perimeter: %i\n", rd->area, g_list_length(rd->contour));
  }
  g_list_free_full(regions, (GDestroyNotify) rd_region_free);
}

/* TODO
 * image struct
 * clean up
 */
