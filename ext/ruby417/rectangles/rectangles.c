#include <stdlib.h> /* malloc, calloc */
#include <math.h> /* sin, cos, atan, pow, sqrt, M_PI, M_PI_2 */
#include "rectangles.h"

static void uf_union(DArray* acc, int32_t a, int32_t b)
{
  int32_t max = (a > b) ? a : b;

  for (int32_t z = acc->len; z <= max; z++) {
    if(!darray_push(acc, INT2PTR(z))) return;
  }

  darray_index_set(acc, uf_find(acc, a), INT2PTR(uf_find(acc, b)));
}

static int32_t uf_find(DArray* acc, int32_t site)
{
  if (site < 0 || site >= acc->len) {
    return -1;
  } else if (darray_index(acc, site) == INT2PTR(site)) {
    return site;
  } else {
    return PTR2INT(darray_index_set(acc, site,
            INT2PTR(uf_find(acc, PTR2INT(darray_index(acc, site))))
           ));
  }
}

static RDMatrix* rd_matrix_new(int16_t width, int16_t height)
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

static RDImage* rd_image_new(uint8_t* data, int16_t width, int16_t height)
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
  /* TODO return max label and take a pointer to label matrix, use to initialize
    regions in rd_extract_regions to avoid resizing                           */
  RDMatrix* labels = rd_matrix_new(image->width, image->height);
  DArray* label_eqvs = darray_new(128);
  if (!labels || !label_eqvs) goto abort;

  int32_t x, y, pixel_label, current_label = 1;
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

  for (int32_t z = 0; z < image->width*image->height; z++) {
    labels->data[z] = uf_find(label_eqvs, labels->data[z]);
  }

  darray_free(label_eqvs, NULL);
  return labels;

abort:

  darray_free(label_eqvs, NULL);
  rd_matrix_free(labels);
  return NULL;
}

static int32_t rd_determine_label(RDImage* image, RDMatrix* labels, DArray* eqvs, int16_t x, int16_t y)
{
  static const int offsets[2][2] = {{-1, 0}, {0, -1}}; /* 4-connectivity */

  uint8_t color = rd_matrix_read_fast(image, x, y);
  int32_t nx, ny, neighbor_label, best_label = 0;

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
    region->boundary = darray_new(8);
    region->cx = 0;
    region->cy = 0;
    region->area = 0;
  }

  return region;
}

static void rd_region_free(RDRegion* region)
{
  darray_free(region->boundary, (DArrayFreeFunc) free);
  free(region);
}

static RDPoint* rd_point_new(int16_t x, int16_t y)
{
  RDPoint* point = malloc(sizeof(RDPoint));

  if (point) {
    point->x = x;
    point->y = y;
  }

  return point;
}

static DArray* rd_extract_regions(RDImage* image, uint8_t intensity_threshold)
{
  DArray* regions = darray_new(16);
  RDMatrix* labels = rd_label_image_regions(image);
  if (!labels) goto abort;

  uint16_t x, y;
  uint32_t label;
  RDRegion* region;

  for (y = 0; y < image->height; y++) {
    for (x = 0; x < image->width; x++) {
      if (rd_matrix_read_fast(image, x, y) >= intensity_threshold) {
        label = rd_matrix_read_fast(labels, x, y);
        region = darray_index(regions, label);

        if (!region) {
          while(regions->len <= label)
            darray_push(regions, NULL);

          region = rd_region_new();

          if (regions->len > label && region) {
            rd_extract_contour(region->boundary, labels, x, y);
            darray_index_set(regions, label, region);
          } else {
            goto abort;
          }
        }

        region->area++;
        region->cx += x;
        region->cy += y;
      }
    }
  }

  uint32_t i = 0;
  while (i < regions->len) {
    region = darray_index(regions, i);

    if (region == NULL) {
      darray_remove_fast(regions, i);
    } else {
      region->cx /= region->area;
      region->cy /= region->area;
      ++i;
    }
  }

abort:

  rd_matrix_free(labels);

  return regions;
}

static void rd_extract_contour(DArray* boundary, RDMatrix* labels, int16_t start_x, int16_t start_y)
{
  const int RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3;

  RDPoint* point;
  int32_t label, target_label = rd_matrix_read_fast(labels, start_x, start_y);
  int direction = DOWN;
  int16_t x = start_x, y = start_y;

  do
    {
      label = rd_matrix_read_safe(labels, x, y, ~target_label);

      if (label == target_label) {
        if (boundary->len == 0 || x != point->x || y != point->y) {
          point = rd_point_new(x, y);
          if(!point) goto abort;

          darray_push(boundary, point);
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

abort:
  /* set errno to distinguish this case from success -- could apply elsewhere */
  return;
}

static DArray* rd_convex_hull(DArray* boundary)
{
  if(boundary->len < 3) return NULL;

  DArray* hull = darray_new(3);
  darray_push(hull, darray_index(boundary, 0));

  darray_msort(boundary, darray_index(boundary, 0), (DArrayCompareFunc) rd_graham_cmp);

  uint32_t p = 1;

  while (p <= boundary->len) {
    if (hull->len == 1
        || rd_vector_cross(darray_index(hull, hull->len-2),
                           darray_index(hull, hull->len-1),
                           darray_index(hull, hull->len-1),
                           darray_index(boundary, p % boundary->len)) > 0) {
      if(++p <= boundary->len) darray_push(hull, darray_index(boundary, p-1));
    } else
      darray_remove_fast(hull, hull->len-1);
  }

  return hull;
}

static int rd_graham_cmp(RDPoint* p, RDPoint* q, RDPoint* base)
{
  return -rd_vector_cross(base, p, base, q);
}

static int32_t rd_vector_dot(RDPoint* a, RDPoint* b, RDPoint* c, RDPoint* d)
{
  return (b->x-a->x)*(d->x-c->x) + (b->y-a->y)*(d->y-c->y);
}

static int32_t rd_vector_cross(RDPoint* a, RDPoint* b, RDPoint* c, RDPoint* d)
{
  return (b->x-a->x)*(d->y-c->y) - (b->y-a->y)*(d->x-c->x);
}

static RDRectangle* rd_fit_rectangle(DArray* hull)
{
  RDRectangle* rectangle = malloc(sizeof(RDRectangle));

  if (rectangle) {
    double min_area = -1, slope, width, height;
    int32_t base_idx = 0, leftmost_idx = base_idx, altitude_idx = -1, rightmost_idx = -1;
    RDPoint* left_base_point, *right_base_point;

    while (base_idx < hull->len)
    {
      left_base_point  = rd_hull_wrap_index(hull, base_idx);
      right_base_point = rd_hull_wrap_index(hull, base_idx+1);

      while (rd_vector_dot(left_base_point,
                           right_base_point,
                           rd_hull_wrap_index(hull, leftmost_idx),
                           rd_hull_wrap_index(hull, leftmost_idx+1)) > 0)
      {
        ++leftmost_idx;
      }

      if(altitude_idx == -1) altitude_idx = leftmost_idx;

      while (rd_vector_cross(left_base_point,
                             right_base_point,
                             rd_hull_wrap_index(hull, altitude_idx),
                             rd_hull_wrap_index(hull, altitude_idx+1)) > 0)
      {
        ++altitude_idx;
      }

      if(rightmost_idx == -1) rightmost_idx = altitude_idx;

      while (rd_vector_dot(left_base_point,
                           right_base_point,
                           rd_hull_wrap_index(hull, rightmost_idx),
                           rd_hull_wrap_index(hull, rightmost_idx+1)) < 0)
      {
        ++rightmost_idx;
      }

      if (left_base_point->x == right_base_point->x) {
        width  = abs(rd_hull_wrap_index(hull, rightmost_idx)->y - rd_hull_wrap_index(hull, leftmost_idx)->y);
        height = abs(rd_hull_wrap_index(hull, altitude_idx)->x - left_base_point->x);
      } else if (left_base_point->y == right_base_point->y) {
        width  = abs(rd_hull_wrap_index(hull, rightmost_idx)->x - rd_hull_wrap_index(hull, leftmost_idx)->x);
        height = abs(rd_hull_wrap_index(hull, altitude_idx)->y - left_base_point->y);
      } else {
        slope = (double) (left_base_point->y - right_base_point->y) / (left_base_point->x - right_base_point->x);
        width = rd_line_distance(rd_hull_wrap_index(hull, leftmost_idx), rd_hull_wrap_index(hull, rightmost_idx), -1/slope);
        height = rd_line_distance(left_base_point, rd_hull_wrap_index(hull, altitude_idx), slope);
      }

      if (width*height < min_area || min_area < 0) {
        RDPoint upper_left, upper_right;
        int16_t dx = right_base_point->x - left_base_point->x,
                dy = right_base_point->y - left_base_point->y;

        if (dx == 0) rectangle->orientation = M_PI_2;
        else         rectangle->orientation = atan((double) dy/dx);

        if (dx < 0 || (dx == 0 && dy < 0)) rectangle->orientation += M_PI;

        rd_determine_fourth_point(left_base_point, right_base_point, rd_hull_wrap_index(hull, leftmost_idx), &upper_left);
        rd_determine_fourth_point(left_base_point, right_base_point, rd_hull_wrap_index(hull, rightmost_idx), &upper_right);
        rectangle->cx = (int16_t) (upper_left.x + upper_right.x - sin(rectangle->orientation)*height) / 2;
        rectangle->cy = (int16_t) (upper_left.y + upper_right.y + cos(rectangle->orientation)*height) / 2;

        rectangle->width = (int16_t) width;
        rectangle->height = (int16_t) height;

        min_area = width*height;
      }

      ++base_idx;
    }
  }

  return rectangle;
}

static RDPoint* rd_hull_wrap_index(DArray* hull, int32_t index)
{
  return darray_index(hull, index % hull->len);
}

static double rd_line_distance(RDPoint* p, RDPoint* q, double slope)
{
  // https://en.wikipedia.org/wiki/Distance_between_two_straight_lines
  return abs((p->y - slope*p->x) - (q->y - slope*q->x)) / sqrt(pow(slope, 2) + 1);
}

static void rd_determine_fourth_point(RDPoint* p1, RDPoint* p2, RDPoint* q1, RDPoint* q2)
{
  if (p1->x == p2->x) {
    q2->x = p1->x;
    q2->y = q1->y;
  } else {
    double slope = (double) (p2->y - p1->y) / (p2->x - p1->x);
    double x = (double) (slope*(q1->y-p1->y) + q1->x + p1->x*pow(slope, 2)) / (pow(slope, 2) + 1);

    q2->x = (int16_t) x;
    q2->y = (int16_t) (slope*(x - p1->x)) + p1->y;
  }
}

#ifndef NO_RUBY

static VALUE mRuby417,
             mRectangleDetection,
             cRectangle;

static VALUE detect_rectangles_wrapper(VALUE self, VALUE data, VALUE width, VALUE height, VALUE area_threshold, VALUE intensity_threshold)
{
  Check_Type(data, T_STRING);
  Check_Type(width, T_FIXNUM);
  Check_Type(height, T_FIXNUM);
  Check_Type(area_threshold, T_FIXNUM);
  Check_Type(intensity_threshold, T_FIXNUM);

  int16_t c_width  = FIX2INT(width),
          c_height = FIX2INT(height);

  if (RSTRING_LEN(data) != c_width*c_height) {
    rb_raise(rb_eEOFError, "image data and dimensions (%ix%i) do not align", c_width, c_height);
  }

  return detect_rectangles((uint8_t*) StringValuePtr(data), c_width, c_height, FIX2INT(area_threshold), (uint8_t) FIX2INT(area_threshold));
}

static VALUE detect_rectangles(uint8_t* data, uint16_t width, uint16_t height, uint32_t area_threshold, uint8_t intensity_threshold)
{
  VALUE rect_data = rb_ary_new();
  RDImage* image = NULL;
  DArray* regions = NULL;

  image = rd_image_new(data, width, height);
  if(!image) goto nomem;

  regions = rd_extract_regions(image, intensity_threshold);
  if(!regions) goto nomem;

  for (uint32_t r = 0; r < regions->len; r++) {
    RDRegion* region = darray_index(regions, r);

    if (region->area >= area_threshold && region->boundary->len >= 3) {
      DArray* hull = rd_convex_hull(region->boundary);
      if(!hull) goto nomem;

      RDRectangle* rect = rd_fit_rectangle(hull);
      if(!rect) {
        darray_free(hull, NULL);
        goto nomem;
      }

      VALUE rect_args[6] = { INT2FIX(rect->cx),    INT2FIX(rect->cy),
                             INT2FIX(rect->width), INT2FIX(rect->height),
                             rb_float_new(rect->orientation), INT2FIX(region->area)};
      VALUE rb_rect = rb_class_new_instance(6, rect_args, cRectangle);

      rb_ary_push(rect_data, rb_rect);

      darray_free(hull, NULL);
      free(rect);
    }
  }

  return rect_data;

nomem:
  free(image);
  darray_free(regions, (DArrayFreeFunc) rd_region_free);
  rb_raise(rb_eNoMemError, "unable to allocate sufficient memory");
}

static VALUE Rectangle_initialize(VALUE self, VALUE cx, VALUE cy, VALUE width, VALUE height, VALUE orientation, VALUE area)
{
  rb_iv_set(self, "@cx", cx);
  rb_iv_set(self, "@cy", cy);
  rb_iv_set(self, "@width", width);
  rb_iv_set(self, "@height", height);
  rb_iv_set(self, "@orientation", orientation);

  rb_iv_set(self, "@region_area", area);

  return self;
}

void Init_rectangle_detection(void)
{
  mRuby417 = rb_define_module("Ruby417");
  mRectangleDetection = rb_define_module_under(mRuby417, "RectangleDetection");
  cRectangle = rb_define_class_under(mRectangleDetection, "Rectangle", rb_cObject);

  rb_define_module_function(mRectangleDetection, "process_image_data", detect_rectangles_wrapper, 5);

  rb_define_method(cRectangle, "initialize", Rectangle_initialize, 6);
  rb_define_attr(cRectangle, "cx", 1, 0);
  rb_define_attr(cRectangle, "cy", 1, 0);
  rb_define_attr(cRectangle, "width", 1, 0);
  rb_define_attr(cRectangle, "height", 1, 0);
  rb_define_attr(cRectangle, "orientation", 1, 0);
  rb_define_attr(cRectangle, "region_area", 1, 0);
}

#endif
