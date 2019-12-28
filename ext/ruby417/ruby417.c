#include <stdlib.h>

#define rd_matrix_read(m, w, h, x, y, fallback)                                 \
  (((x) >= 0 && (y) >= 0 && (x) < (w) && (y) < (h)) ?                           \
    (m)[(x)+(y)*(w)]                                                            \
  :                                                                             \
    (fallback))

#define rd_matrix_set(m, w, h, x, y, val) do {                                  \
  if((x) >= 0 && (y) >= 0 && (x) < (w) && (y) < (h))                            \
    (m)[(x)+(y)*(w)] = (val);                                                   \
} while(0)

#define UF_GROWTH_FACTOR 2

typedef struct UFConnections {
  int *sites;
  int size;
} UFConnections;

static UFConnections *uf_alloc_connections(int);
static void uf_free_connections(UFConnections*);
static _Bool uf_resize_connections(UFConnections*);
static void uf_union(UFConnections*, int, int);
static int uf_find(UFConnections*, int);

static int *rd_label_image_regions(char*, int, int);
static int rd_determine_label(char*, int*, int, int, UFConnections*, int, int);

static RDRegion *rd_extract_regions(char*, int, int, int);

static UFConnections *uf_alloc_connections(int size){
  UFConnections *conns = malloc(sizeof(UFConnections));

  if(conns){
    conns->size = size;
    conns->sites = malloc(sizeof(int) * size);

    if(conns->sites){
      for(int i=0; i<size; i++)
        conns->sites[i] = i;
    } else {
      free(conns);
      return NULL;
    }
  }

  return conns;
}

static void uf_free_connections(UFConnections *connections){
  free(connections->sites);
  free(connections);
}

static _Bool uf_resize_connections(UFConnections *connections){
  int *new_connections = realloc(connections->sites, sizeof(int)*connections->size*UF_GROWTH_FACTOR);

  if(new_connections){
    for(int i=connections->size; i<connections->size*UF_GROWTH_FACTOR; i++)
      new_connections[i] = i;

    connections->sites = new_connections;
    connections->size *= UF_GROWTH_FACTOR;
    return 1;
  }

  return 0;
}

static void uf_union(UFConnections *connections, int a, int b){
  if(a < connections->size && b < connections->size)
    connections->sites[a] = connections->sites[b];
}

static int uf_find(UFConnections *connections, int site){
  if(connections->sites[site] == site)
    return site;
  return connections->sites[site] = uf_find(connections, connections->sites[site]);
}

static int *rd_label_image_regions(char *image, int width, int height){
  int *labels = calloc(width*height, sizeof(int));
  UFConnections *label_eqvs = uf_alloc_connections(width);
  if(!(labels && label_eqvs)) goto hard_exit;

  int x, y, pixel_label, current_label = 1;
  for(y=0; y<height; y++){
    for(x=0; x<width; x++){
      pixel_label = rd_determine_label(image, labels, width, height, label_eqvs, x, y);

      if(pixel_label) {
        rd_matrix_set(labels, width, height, x, y, pixel_label);
      } else {
        rd_matrix_set(labels, width, height, x, y, current_label++);

        if(current_label == label_eqvs->size)
          if(!uf_resize_connections(label_eqvs)) goto soft_exit;
      }
    }
  }

soft_exit:
  for(int z=0; z<width*height; z++)
    labels[z] = uf_find(label_eqvs, labels[z]);

hard_exit:
  if(label_eqvs) uf_free_connections(label_eqvs);

  return labels;
}

static int rd_determine_label(char *image, int *labels, int width, int height, UFConnections *eqvs, int x, int y){
  static int offsets[2][2] = {{-1, 0}, {0, -1}}; /* 4-connectivity */

  char color = rd_matrix_read(image, width, height, x, y, 0);
  int nx, ny, neighbor_label, best_label = 0;
  for(int i=0; i<2; i++){
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
