#ifndef RECTANGLES_H
#define RECTANGLES_H

#include <stdbool.h>
#include <stddef.h> // size_t
#include "image.h"
#include "darray.h"

struct point {
  int x, y;
};

struct region {
  struct darray *boundary;
  long cx, cy;
  long area;
};

struct rectangle {
  int cx, cy;
  int width, height;
  long fill;
  double orientation;
};

struct rectangle_pair {
  struct rectangle *one;
  struct rectangle *two;
  double score;
};

struct pairing_settings {
  long area_threshold;
  double rectangularity_threshold;
  double angle_variation_threshold;
  double area_variation_threshold;
  double width_variation_threshold;
  double height_variation_threshold;
  int guard_aspect_min, guard_aspect_max;
  int barcode_aspect_min, barcode_aspect_max;
};

static struct point *point_new(int x, int y, void *(*malloc)(size_t size));
static struct region *region_new(void *(*malloc)(size_t size),
                                 void *(*realloc)(void *ptr, size_t new_size),
                                 void (*free)(void *ptr));
static void region_shallow_free(struct region *reg);
static void region_free(struct region *reg);
static struct image32 *image_label_regions(struct image8 *im,
                                           void *(*malloc)(size_t size),
                                           void *(*realloc)(void *ptr, size_t new_size),
                                           void (*free)(void *ptr));
static bool image_follow_contour(struct image8 *im, struct darray *boundary, int start_x, int start_y);
static struct darray *image_extract_regions(struct image8 *image,
                                            struct image32 *labeled,
                                            void *(*malloc)(size_t size),
                                            void *(*realloc)(void *ptr, size_t new_size),
                                            void (*free)(void *ptr));
static bool boundary_convex_hull(struct darray *boundary, struct darray *hull);
static void hull_minimal_rectangle(struct darray *hull, long fill, struct rectangle *rect);
static bool pair_aligned_rectangles(struct pairing_settings *settings, struct darray *rects, struct darray *pairs);

#endif
