#include <math.h> // sin, cos
#include <stdlib.h> // NULL
#include <stdbool.h>
#include "image.h"

static struct image8 *image8_new(int width, int height, void *(*malloc)(size_t size), void (*free)(void *ptr)) {
  struct image8 *im = malloc(sizeof(*im));

  if (im) {
    im->width = width;
    im->height = height;
    im->free = free;
    im->data = malloc(sizeof(*im->data)*width*height);

    if (!im->data) {
      free(im);
      return NULL;
    }
  }

  return im;
}

static void image8_free(struct image8 *im) {
  if (im) {
    im->free(im->data);
    im->free(im);
  }
}

static void image8_set(struct image8 *im, int x, int y, unsigned char val) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    im->data[im->width*y + x] = val;
  }
}

static unsigned char image8_get(struct image8 *im, int x, int y) {
  return im->data[im->width*y + x];
}

static unsigned char image8_get_with_fallback(struct image8 *im, int x, int y, unsigned char fallback) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    return image8_get(im, x, y);
  }
  return fallback;
}

static struct image32 *image32_new(int width, int height, void *(*malloc)(size_t size), void (*free)(void *ptr)) {
  struct image32 *im = malloc(sizeof(*im));

  if (im) {
    im->width = width;
    im->height = height;
    im->free = free;
    im->data = malloc(sizeof(*im->data)*width*height);

    if (!im->data) {
      free(im);
      return NULL;
    }
  }

  return im;
}

static void image32_free(struct image32 *im) {
  if (im) {
    im->free(im->data);
    im->free(im);
  }
}

static void image32_set(struct image32 *im, int x, int y, unsigned val) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    im->data[im->width*y + x] = val;
  }
}

static unsigned image32_get(struct image32 *im, int x, int y) {
  return im->data[im->width*y + x];
}

static unsigned image32_get_with_fallback(struct image32 *im, int x, int y, unsigned fallback) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    return image32_get(im, x, y);
  }
  return fallback;
}

static struct point *point_new(int x, int y, void *(*malloc)(size_t size)) {
  struct point *pt = malloc(sizeof(*pt));
  if (pt) {
    pt->x = x;
    pt->y = y;
  }
  return pt;
}

static void point_rotate(struct point *p, struct point *origin, double angle, struct point *out) {
  int x = p->x, y = p->y;
  out->x = (int) round(origin->x+(x-origin->x)*cos(angle)-(y-origin->y)*sin(angle));
  out->y = (int) round(origin->y+(x-origin->x)*sin(angle)+(y-origin->y)*cos(angle));
}

static struct region *region_new(void *(*malloc)(size_t size),
                                 void *(*realloc)(void *ptr, size_t new_size),
                                 void (*free)(void *ptr)) {
  struct region *reg = malloc(sizeof(*reg));
  if (reg) {
    reg->cx = 0;
    reg->cy = 0;
    reg->area = 0;
    reg->boundary = darray_new(16, free, malloc, realloc, free);

    if (!reg->boundary) {
      free(reg);
      return NULL;
    }
  }
  return reg;
}

static void region_shallow_free(struct region *reg) {
  if (reg) {
    void (*free)(void *ptr) = reg->boundary->free;
    darray_free(reg->boundary, false);
    free(reg);
  }
}

static void region_free(struct region *reg) {
  if (reg) {
    void (*free)(void *ptr) = reg->boundary->free;
    darray_free(reg->boundary, true);
    free(reg);
  }
}

static long uf_find(struct darray *acc, long x) {
  if (x < 0 || x >= acc->len) {
    return -1;
  } else if ((long) darray_index(acc, x) == x) {
    return x;
  } else {
    return (long) darray_index_set(acc, x, (void *) uf_find(acc, (long) darray_index(acc, x)));
  }
}

static bool uf_union(struct darray *acc, unsigned a, unsigned b) {
  unsigned max = (a > b) ? a : b;
  for (long z = acc->len; z <= max; z++) {
    if(!darray_push(acc, (void *) z)) return false;
  }
  darray_index_set(acc, uf_find(acc, a), (void *) uf_find(acc, b));
  return true;
}

static unsigned determine_label(struct image8 *im, struct image32 *labeled,
                                struct darray *equivs, bool *oom,
                                unsigned x, unsigned y) {
  static const int offsets[2][2] = {{-1, 0}, {0, -1}}; /* 4-connectivity */

  unsigned char color = image8_get(im, x, y);
  unsigned label = 0;
  for (int i = 0; i < 2; i++) {
    int nx = offsets[i][0] + x, ny = offsets[i][1] + y;
    if (color == image8_get_with_fallback(im, nx, ny, ~color)) {
      unsigned neighbor_label = image32_get(labeled, nx, ny);
      if (label && label != neighbor_label) {
        if (!uf_union(equivs, label, neighbor_label)) {
          *oom = true;
          break;
        }
      } else if (label == 0 || neighbor_label < label) {
        label = neighbor_label;
      }
    }
  }

  return label;
}

static struct image32 *image_label_regions(struct image8 *im,
                                           void *(*malloc)(size_t size),
                                           void *(*realloc)(void *ptr, size_t new_size),
                                           void (*free)(void *ptr)) {
  struct image32 *labeled = image32_new(im->width, im->height, malloc, free);
  struct darray *equivs = darray_new(128, NULL, malloc, realloc, free);
  if (!labeled || !equivs) goto oom;

  unsigned current_label = 1;
  bool oom = false;
  for (int y = 0; y < im->height; y++) {
    for (int x = 0; x < im->width; x++) {
      unsigned label = determine_label(im, labeled, equivs, &oom, x, y);
      if (oom) {
        goto oom;
      } else if (label) {
        image32_set(labeled, x, y, label);
      } else {
        image32_set(labeled, x, y, current_label++);
        uf_union(equivs, current_label, current_label);
      }
    }
  }

  for (long z = 0; z < (long)im->width*im->height; z++) {
    long label = uf_find(equivs, labeled->data[z]);
    if (label > 0) labeled->data[z] = (unsigned) label;
  }

  darray_free(equivs, false);
  return labeled;

oom:
  darray_free(equivs, false);
  image32_free(labeled);
  return NULL;
}

static bool is_contour_pixel(struct image8 *im, int x, int y, unsigned char target) {
  return target != image8_get_with_fallback(im, x-1, y, ~target) ||
         target != image8_get_with_fallback(im, x+1, y, ~target) ||
         target != image8_get_with_fallback(im, x, y-1, ~target) ||
         target != image8_get_with_fallback(im, x, y+1, ~target);
}

static bool image_follow_contour(struct image8 *im, struct darray *boundary, int start_x, int start_y) {
  static const int RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3;
  struct point* point = NULL;
  unsigned char target = image8_get(im, start_x, start_y);
  int direction = DOWN, x = start_x, y = start_y;
  do {
    unsigned char color = image8_get_with_fallback(im, x, y, ~target);

    if (color == target) {
      if ((!point || x != point->x || y != point->y) && is_contour_pixel(im, x, y, target)) {
        point = point_new(x, y, boundary->malloc);
        if (!point || !darray_push(boundary, point)) {
          if (boundary->eltfree) boundary->eltfree(point);
          else boundary->free(point);

          return false;
        }
      }
      direction = (direction - 1) & 3; // left turn
    } else {
      direction = (direction + 1) & 3; // right turn
    }

    if      (direction == RIGHT) x++;
    else if (direction == DOWN)  y++;
    else if (direction == LEFT)  x--;
    else if (direction == UP)    y--;
  } while (x != start_x || y != start_y);

  return true;
}

static void region_free_wrapper(void *ptr) {
  region_free((struct region *) ptr);
}

static struct darray *image_extract_regions(struct image8 *image,
                                            struct image32 *labeled,
                                            void *(*malloc)(size_t size),
                                            void *(*realloc)(void *ptr, size_t new_size),
                                            void (*free)(void *ptr)) {
  struct darray *regions = darray_new(16, region_free_wrapper, malloc, realloc, free);

  if (regions) {
    for (int y = 0; y < labeled->height; y++) {
      for (int x = 0; x < labeled->width; x++) {
        unsigned label = image32_get(labeled, x, y);
        struct region *region = darray_index(regions, label);

        if (!region) {
          for (unsigned z = regions->len; z <= label; z++) {
            if (!darray_push(regions, NULL)) goto oom;
          }

          if ((region=region_new(malloc, realloc, free))) {
            darray_index_set(regions, label, region);

            if (!image_follow_contour(image, region->boundary, x, y)) {
              goto oom;
            }
          } else {
            goto oom;
          }
        }

        region->area++;
        region->cx += x;
        region->cy += y;
      }
    }

    unsigned i = 0;
    while (i < regions->len) {
      struct region *region = darray_index(regions, i);

      if (region) {
        region->cx /= region->area;
        region->cy /= region->area;
        ++i;
      } else {
        darray_remove_fast(regions, i);
      }
    }
  }

  return regions;

oom:
  darray_free(regions, true);
  return NULL;
}
