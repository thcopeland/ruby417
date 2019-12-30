#include <glib.h>

#define rd_matrix_read(m, w, h, x, y, fallback)                                 \
  (((x) >= 0 && (y) >= 0 && (x) < (w) && (y) < (h)) ?                           \
    (m)[(x)+(y)*(w)]                                                            \
  :                                                                             \
    (fallback))

#define rd_matrix_set(m, w, h, x, y, val) do {                                  \
  if((x) >= 0 && (y) >= 0 && (x) < (w) && (y) < (h))                            \
    (m)[(x)+(y)*(w)] = (val);                                                   \
} while(0)

static void uf_union(GArray*, gint32, gint32);
static gint32 uf_find(GArray*, gint32);

static gint32 *rd_label_image_regions(guint8*, gint32, gint32);
static gint32 rd_determine_label(guint8*, gint32*, gint32, gint32, GArray*, gint32, gint32);

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
  if(!labels || !label_eqvs) goto fast_exit;

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

fast_exit:
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
