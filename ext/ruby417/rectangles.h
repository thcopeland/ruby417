#ifndef RECTANGLES_H
#define RECTANGLES_H

#include <stdbool.h>
#include "image.h"
#include "darray.h"

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

struct barcode_corners {
  struct point upper_left;
  struct point upper_right;
  struct point lower_left;
  struct point lower_right;
};

static bool boundary_convex_hull(struct darray *boundary, struct darray *hull);
static void hull_minimal_rectangle(struct darray *hull, long fill, struct rectangle *rect);
static bool pair_aligned_rectangles(struct pairing_settings *settings, struct darray *rects, struct darray *pairs);
static void determine_barcode_corners(struct rectangle_pair *pair, struct barcode_corners *corners);

#endif
