#include <math.h> // sin, cos, atan2, round, sqrt, hypot, M_PI, M_PI_2
#include <stdlib.h> // abs, NULL
#include "rectangles.h"

static long vec_dot(struct point *a, struct point *b, struct point *c, struct point *d) {
  return (long) (b->x-a->x)*(d->x-c->x) +  (long) (b->y-a->y)*(d->y-c->y);
}

static long vec_cross(struct point *a, struct point *b, struct point *c, struct point *d) {
  return (long) (b->x-a->x)*(d->y-c->y) - (long) (b->y-a->y)*(d->x-c->x);
}

static bool boundary_convex_hull(struct darray *boundary, struct darray *hull) {
  if (boundary->len > 0) {
    // Since (by construction) it is the left-most point with the highest y-value,
    // the first point in the boundary is also in the convex hull.
    if (!darray_push(hull, darray_index(boundary, 0))) return false;

    for (unsigned i = 1; i <= boundary->len; i++) {
      struct point *p = darray_index(boundary, i % boundary->len);
      while (hull->len > 1 && vec_cross(darray_index(hull, hull->len-1), darray_index(hull, hull->len-2), darray_index(hull, hull->len-1), p) >= 0) {
        darray_pop(hull);
      }

      if (i < boundary->len && !darray_push(hull, p)) return false;
    }
  }

  return true;
}

static struct point *hull_wrap_index(struct darray *hull, unsigned index) {
  return darray_index(hull, index % hull->len);
}

// Returns the perpendicular distance between q and a line of slope slope through p.
static double line_distance(struct point *p, struct point *q, double slope) {
  return fabs((p->y - slope*p->x) - (q->y - slope*q->x)) / sqrt(slope*slope + 1);
}

// Given two points, p1 and p2, on the same line, and a third point, q1 that
// lies on another line perpendicular to the first, determines the intersection
// point of the lines, q2.
static void determine_fourth_point(struct point *p1, struct point *p2, struct point *q1, struct point *q2) {
  if (p1->x == p2->x) {
    q2->x = p1->x;
    q2->y = q1->y;
  } else {
    double slope = (double) (p2->y - p1->y) / (p2->x - p1->x),
           slope2 = slope*slope,
           x = (slope*(q1->y-p1->y) + q1->x + p1->x*slope2) / (slope2 + 1);

    q2->x = (unsigned) round(x);
    q2->y = (unsigned) round(slope*(x - p1->x)) + p1->y;
  }
}

static void hull_minimal_rectangle(struct darray *hull, long fill, struct rectangle *rect) {
  double min_area = -1, slope, width, height;
  unsigned base_idx = 0, leftmost_idx = base_idx, altitude_idx = 0, rightmost_idx = 0;
  struct point *left_base_point, *right_base_point;

  while (base_idx < hull->len) {
    // one edge of the hull
    left_base_point  = hull_wrap_index(hull, base_idx);
    right_base_point = hull_wrap_index(hull, base_idx+1);

    // find the point on the hull that is furthest to the left from the edge
    while (vec_dot(left_base_point, right_base_point, hull_wrap_index(hull, leftmost_idx), hull_wrap_index(hull, leftmost_idx+1)) > 0) ++leftmost_idx;

    // find the point furthest from the edge in the perpendicular direction
    if(altitude_idx == 0) altitude_idx = leftmost_idx;
    while (vec_cross(left_base_point, right_base_point, hull_wrap_index(hull, altitude_idx), hull_wrap_index(hull, altitude_idx+1)) > 0) ++altitude_idx;

    // find the point that is furthest to the right from the edge
    if(rightmost_idx == 0) rightmost_idx = altitude_idx;
    while (vec_dot(left_base_point, right_base_point, hull_wrap_index(hull, rightmost_idx), hull_wrap_index(hull, rightmost_idx+1)) < 0) ++rightmost_idx;

    // Determine the dimensions described by these points. If the edge is vertical
    // or horizontal, the scenario must be handled separately to avoid division by zero.
    // The +1's are needed (I think) to counter the fencepost issue between discrete
    // pixels and the true geometric distance.
    // Note: The width dimension is along the orientation vector.
    // RectangleDetection::Rectangle#normalize! relies on this.
    if (left_base_point->x == right_base_point->x) {
      width  = abs(hull_wrap_index(hull, rightmost_idx)->y - hull_wrap_index(hull, leftmost_idx)->y)+1;
      height = abs(hull_wrap_index(hull, altitude_idx)->x - left_base_point->x)+1;
    } else if (left_base_point->y == right_base_point->y) {
      width  = abs(hull_wrap_index(hull, rightmost_idx)->x - hull_wrap_index(hull, leftmost_idx)->x)+1;
      height = abs(hull_wrap_index(hull, altitude_idx)->y - left_base_point->y)+1;
    } else {
      slope = (double) (left_base_point->y - right_base_point->y) / (left_base_point->x - right_base_point->x);
      width = line_distance(hull_wrap_index(hull, leftmost_idx), hull_wrap_index(hull, rightmost_idx), -1/slope)+1;
      height = line_distance(left_base_point, hull_wrap_index(hull, altitude_idx), slope)+1;
    }

    if (width*height < min_area || min_area < 0) {
      struct point upper_left, upper_right;
      double orientation = atan2(right_base_point->y - left_base_point->y, right_base_point->x - left_base_point->x);

      // use two corner pixels along the base side to determine the rect's center
      determine_fourth_point(left_base_point, right_base_point, hull_wrap_index(hull, leftmost_idx), &upper_left);
      determine_fourth_point(left_base_point, right_base_point, hull_wrap_index(hull, rightmost_idx), &upper_right);
      rect->cx = (int) (upper_left.x + upper_right.x - sin(orientation)*height) / 2;
      rect->cy = (int) (upper_left.y + upper_right.y + cos(orientation)*height) / 2;
      rect->fill = fill;

      // normalize the orientation to be between 0 and pi/2
      if (orientation < 0) orientation += M_PI;
      if (orientation < M_PI_2) {
        rect->orientation = orientation;
        rect->width = (int) width;
        rect->height = (int) height;
      } else {
        rect->orientation = orientation - M_PI_2;
        rect->width = (int) height;
        rect->height = (int) width;
      }

      // normalize again so that height >= width, orientation is between 0 and pi, and
      // the orientation is along the width
      if (rect->width > rect->height) {
        int tmp = rect->width;
        rect->width = rect->height;
        rect->height = tmp;
        rect->orientation += M_PI_2;
      }

      min_area = width*height;
    }

    ++base_idx;
  }
}

static int rect_cmp_by_area(const void *a, const void *b, void *ctx) {
  (void) ctx;
  return ((struct rectangle*) a)->fill - ((struct rectangle*) b)->fill;
}

static int pair_cmp_by_score(const void *a, const void *b, void *ctx) {
  (void) ctx;
  return ((struct rectangle_pair*) a)->score < ((struct rectangle_pair*) b)->score ? 1 : -1;
}

static bool rect_qualifies(struct pairing_settings *settings, struct rectangle *rect) {
  return (long) rect->width*rect->height >= settings->area_threshold &&
         (double) rect->fill/(rect->width*rect->height) >= settings->rectangularity_threshold &&
         rect->height >= rect->width*settings->guard_aspect_min &&
         rect->height <= rect->width*settings->guard_aspect_max;
}

static bool rect_pair_qualifies(struct pairing_settings *settings, struct rectangle *one, struct rectangle *two) {
  double joining_angle = atan2(one->cy-two->cy, one->cx-two->cx),
         barcode_width = hypot(one->cx-two->cx, one->cy-two->cy),
         barcode_height = (one->height + two->height)/2,
         average_width = (one->width + two->width)/2,
         average_area = ((double) one->width*one->height+two->width*two->height)/2;

  return rect_qualifies(settings, one) &&
         rect_qualifies(settings, two) &&
         abs(one->width*one->height-two->width*two->height) <= average_area*settings->area_variation_threshold &&
         abs(one->width-two->width) <= average_width*settings->width_variation_threshold &&
         abs(one->height-two->height) <= barcode_height*settings->height_variation_threshold &&
         // using sin here as a "distance from pi/2 and 3pi/2" approximation
         fabs(sin(one->orientation-two->orientation)) <= settings->angle_variation_threshold &&
         fabs(sin(joining_angle-one->orientation)) <= settings->angle_variation_threshold &&
         fabs(sin(joining_angle-two->orientation)) <= settings->angle_variation_threshold &&
         barcode_width >= barcode_height*settings->barcode_aspect_min &&
         barcode_width <= barcode_height*settings->barcode_aspect_max;
}

static double scale_score(double x) {
  // somewhat arbitrary function to restrict range to (0, 1], with maximum at 0
  return 1/(x*x+1);
}

static double score_rect_pair(struct rectangle *one, struct rectangle *two) {
  double joining_angle = atan2(one->cy-two->cy, one->cx-two->cx),
         barcode_width = hypot(one->cx-two->cx, one->cy-two->cy),
         barcode_height = (one->height + two->height)/2,
         average_width = (one->width + two->width)/2,
         average_area = ((double) one->width*one->height+two->width*two->height)/2;

  double rectangularity_score = (double) (one->fill*two->fill)/(one->width*one->height*two->width*two->height),
         area_variation_score = scale_score(abs(one->width*one->height-two->width*two->height)/average_area/16),
         dimension_diff_score = scale_score(abs(one->width-two->width)/average_width/8+abs(one->height-two->height)/barcode_height/2),
         angle_variation_score = scale_score(4*sin(one->orientation-two->orientation)),
         joining_angle_score = scale_score(2*fabs(sin(joining_angle-one->orientation)) + 2*fabs(sin(joining_angle-two->orientation))),
         guard_aspect_score =  barcode_height/average_width > 3 ? 1.0 : 0.9,
         guard_area_score = 1 - 1/sqrt(average_area),
         barcode_aspect_score = barcode_width/barcode_height > 2 ? 1.0 : 0.9;

  return rectangularity_score * area_variation_score * dimension_diff_score * angle_variation_score * joining_angle_score * guard_aspect_score * guard_area_score * barcode_aspect_score;
}

static bool pair_aligned_rectangles(struct pairing_settings *settings, struct darray *rects, struct darray *pairs) {
  darray_qsort(rects, NULL, rect_cmp_by_area);

  for (unsigned i = 0; (long) i < (long) rects->len-1; i++) {
    struct rectangle *one = darray_index(rects, i);
    if (!rect_qualifies(settings, one)) continue;

    for (unsigned j = i+1; j < rects->len; j++) {
      struct rectangle *two = darray_index(rects, j);

      if (rect_pair_qualifies(settings, one, two)) {
        struct rectangle_pair *pair = pairs->malloc(sizeof(*pair));
        if (!pair) return false;
        pair->one = one;
        pair->two = two;
        pair->score = score_rect_pair(one, two);

        if (!darray_push(pairs, pair)) {
          pairs->free(pair);
          return false;
        }
      }
    }
  }

  darray_qsort(pairs, NULL, pair_cmp_by_score);

  return true;
}

static int min_point_by_coords(struct point *points, int count, int k, int l) {
  int best_index = 0, best_value = points[0].x*k + points[0].y*l;
  for (int i = 1; i < count; i++) {
    int value = points[i].x*k + points[i].y*l;
    if (value < best_value) {
      best_index = i;
      best_value = value;
    }
  }
  return best_index;
}

static void determine_barcode_corners(struct rectangle_pair *pair, struct barcode_corners *corners) {
  double barcode_orientation = atan2(pair->one->cy-pair->two->cy, pair->one->cx-pair->two->cx);
  if (barcode_orientation < 0) barcode_orientation += M_PI;

  struct point barcode_center = {
    .x=pair->one->cx+(pair->two->cx-pair->one->cx)/2,
    .y=pair->one->cy+(pair->two->cy-pair->one->cy)/2
  };
  struct point rectangle_centers[2] = {
    { .x=pair->one->cx, .y=pair->one->cy },
    { .x=pair->two->cx, .y=pair->two->cy }
  };
  double rectangle_orientations[2] = { pair->one->orientation, pair->two->orientation };
  struct point transformed[8], rect_points[8] = {
    { .x=pair->one->cx-(pair->one->width+1)/2, .y=pair->one->cy-(pair->one->height+1)/2 }, // (...+1)/2 reduces rounding errors compared to .../2
    { .x=pair->one->cx-(pair->one->width+1)/2, .y=pair->one->cy+(pair->one->height+1)/2 },
    { .x=pair->one->cx+(pair->one->width+1)/2, .y=pair->one->cy+(pair->one->height+1)/2 },
    { .x=pair->one->cx+(pair->one->width+1)/2, .y=pair->one->cy-(pair->one->height+1)/2 },

    { .x=pair->two->cx-(pair->two->width+1)/2, .y=pair->two->cy-(pair->two->height+1)/2 },
    { .x=pair->two->cx-(pair->two->width+1)/2, .y=pair->two->cy+(pair->two->height+1)/2 },
    { .x=pair->two->cx+(pair->two->width+1)/2, .y=pair->two->cy+(pair->two->height+1)/2 },
    { .x=pair->two->cx+(pair->two->width+1)/2, .y=pair->two->cy-(pair->two->height+1)/2 }
  };
  for (int i = 0; i < 8; i++) {
    // rotate all the points to the correct position
    point_rotate(&rect_points[i], &rectangle_centers[i/4], rectangle_orientations[i/4], &rect_points[i]);
    // rotate them again to reverse the overall barcode orientation
    point_rotate(&rect_points[i], &barcode_center, -barcode_orientation, &transformed[i]);
    // point_rotate(&transformed[i], &rectangle_centers[i/4], barcode_orientation-rectangle_orientations[i/4], &transformed[i]); // not sure about this
  }

  // find the corners
  corners->upper_left = rect_points[min_point_by_coords(transformed, 8, 1, 1)];
  corners->upper_right = rect_points[min_point_by_coords(transformed, 8, -1, 1)];
  corners->lower_left = rect_points[min_point_by_coords(transformed, 8, 1, -1)];
  corners->lower_right = rect_points[min_point_by_coords(transformed, 8, -1, -1)];
}
