#include <stdlib.h> /* malloc, calloc */
#include <math.h> /* sin, cos, atan, pow, sqrt, M_PI, M_PI_2 */
#include "rectangles.h"

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
  if (!labels) goto abort;

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
    region->boundary = g_array_new(FALSE, FALSE, sizeof(RDPoint));
    region->cx = 0;
    region->cy = 0;
    region->area = 0;
  }

  return region;
}

static void rd_region_free(RDRegion* region)
{
  g_array_free(region->boundary, TRUE);
  free(region);
}

static GPtrArray* rd_extract_regions(RDImage* image, guint8 intensity_threshold)
{
  GPtrArray* regions = g_ptr_array_new_with_free_func((GDestroyNotify) rd_region_free);
  GHashTable* region_lookup = g_hash_table_new(g_direct_hash, g_direct_equal);
  RDMatrix* labels = rd_label_image_regions(image);
  if (!labels) goto abort;

  gint16 x, y;
  gint32 label;
  gpointer lookup_key;
  RDRegion* region;
  RDPoint point;

  for (y = 0; y < image->height; y++) {
    for (x = 0; x < image->width; x++) {
      if (rd_matrix_read_fast(image, x, y) >= intensity_threshold) {
        label = rd_matrix_read_fast(labels, x, y);
        lookup_key = GINT_TO_POINTER(label);
        region = g_hash_table_lookup(region_lookup, lookup_key);

        if (!region) {
          region = rd_region_new();
          if (!region) goto abort;

          g_hash_table_insert(region_lookup, lookup_key, region);
          g_ptr_array_add(regions, region);
        }

        region->area++;
        region->cx += x;
        region->cy += y;

        if (x == 0 || y == 0 || x == image->width-1 || y == image->height-1 ||
            rd_matrix_read_fast(labels, x-1, y) != label ||
            rd_matrix_read_fast(labels, x, y-1) != label ||
            rd_matrix_read_fast(labels, x+1, y) != label ||
            rd_matrix_read_fast(labels, x, y+1) != label)
        {
          point.x = x;
          point.y = y;

          g_array_append_val(region->boundary, point);
        }
      }
    }
  }

  for(int i = 0; i < regions->len; i++) {
    region = g_ptr_array_index(regions, i);
    region->cx /= region->area;
    region->cy /= region->area;
  }

abort:

  rd_matrix_free(labels);
  g_hash_table_destroy(region_lookup);

  return regions;
}

static GArray* rd_convex_hull(GArray* boundary)
{
  if(boundary->len < 3) return NULL;

  GArray* hull = g_array_new(FALSE, FALSE, sizeof(RDPoint));
  g_array_append_val(hull, g_array_index(boundary, RDPoint, 0));

  g_array_sort_with_data(boundary, (GCompareDataFunc) rd_graham_cmp, &g_array_index(boundary, RDPoint, 0));

  gint32 p = 1;

  while (p <= boundary->len) {
    if (hull->len == 1
        || rd_vector_cross(&g_array_index(hull, RDPoint, hull->len-2),
                           &g_array_index(hull, RDPoint, hull->len-1),
                           &g_array_index(hull, RDPoint, hull->len-1),
                           &g_array_index(boundary, RDPoint, p % boundary->len)) > 0) {
      if(++p <= boundary->len) g_array_append_val(hull, g_array_index(boundary, RDPoint, p-1));
    } else
      g_array_remove_index(hull, hull->len-1);
  }

  return hull;
}

static int rd_graham_cmp(RDPoint* p, RDPoint* q, RDPoint* base)
{
  return -rd_vector_cross(base, p, base, q);
}

static gint32 rd_vector_dot(RDPoint* a, RDPoint* b, RDPoint* c, RDPoint* d)
{
  return (b->x-a->x)*(d->x-c->x) + (b->y-a->y)*(d->y-c->y);
}

static gint32 rd_vector_cross(RDPoint* a, RDPoint* b, RDPoint* c, RDPoint* d)
{
  return (b->x-a->x)*(d->y-c->y) - (b->y-a->y)*(d->x-c->x);
}

static RDRectangle* rd_fit_rectangle(GArray* hull)
{
  RDRectangle* rectangle = malloc(sizeof(RDRectangle));

  if (rectangle) {
    double min_area = G_MAXDOUBLE, slope, width, height;
    gint32 base_idx = 0, leftmost_idx = base_idx, altitude_idx = -1, rightmost_idx = -1;
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

      if (width*height < min_area) {
        RDPoint upper_left, upper_right;
        gint16 dx = right_base_point->x - left_base_point->x,
               dy = right_base_point->y - left_base_point->y;

        if (dx == 0) rectangle->orientation = M_PI_2;
        else         rectangle->orientation = atan((double) dy/dx);

        if (dx < 0 || (dx == 0 && dy < 0)) rectangle->orientation += M_PI;

        rd_determine_fourth_point(left_base_point, right_base_point, rd_hull_wrap_index(hull, leftmost_idx), &upper_left);
        rd_determine_fourth_point(left_base_point, right_base_point, rd_hull_wrap_index(hull, rightmost_idx), &upper_right);
        rectangle->cx = (gint16) (upper_left.x + upper_right.x - sin(rectangle->orientation)*height) / 2;
        rectangle->cy = (gint16) (upper_left.y + upper_right.y + cos(rectangle->orientation)*height) / 2;

        rectangle->width = (gint16) width;
        rectangle->height = (gint16) height;

        min_area = width*height;
      }

      ++base_idx;
    }
  }

  return rectangle;
}

static RDPoint* rd_hull_wrap_index(GArray* hull, gint index)
{
  return &g_array_index(hull, RDPoint, index % hull->len);
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

    q2->x = (gint16) x;
    q2->y = (gint16) (slope*(x - p1->x)) + p1->y;
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

  gint16 c_width  = FIX2INT(width),
         c_height = FIX2INT(height);

  if (RSTRING_LEN(data) != c_width*c_height) {
    rb_raise(rb_eEOFError, "image data and dimensions (%ix%i) do not align", c_width, c_height);
  }

  return detect_rectangles((unsigned char*) StringValuePtr(data), c_width, c_height, FIX2INT(area_threshold), (guint8) FIX2INT(area_threshold));
}

static VALUE detect_rectangles(guint8* data, gint16 width, gint16 height, gint32 area_threshold, guint8 intensity_threshold)
{
  VALUE rect_data = rb_ary_new();
  RDImage* image = NULL;
  GPtrArray* regions = NULL;

  image = rd_image_new(data, width, height);
  if(!image) goto nomem;

  regions = rd_extract_regions(image, intensity_threshold);
  if(!regions) goto nomem;

  for (int r = 0; r < regions->len; r++) {
    RDRegion* region = g_ptr_array_index(regions, r);

    if (region->area >= area_threshold && region->boundary->len >= 3) {
      GArray* hull = rd_convex_hull(region->boundary);
      if(!hull) goto nomem;

      RDRectangle* rect = rd_fit_rectangle(hull);
      if(!rect) {
        g_array_free(hull, TRUE);
        goto nomem;
      }

      VALUE rect_args[6] = { INT2FIX(rect->cx),    INT2FIX(rect->cy),
                             INT2FIX(rect->width), INT2FIX(rect->height),
                             rb_float_new(rect->orientation), INT2FIX(region->area)};
      VALUE rb_rect = rb_class_new_instance(6, rect_args, cRectangle);

      rb_ary_push(rect_data, rb_rect);

      g_array_free(hull, TRUE);
      free(rect);
    }
  }

  return rect_data;

nomem:
  free(image);
  g_ptr_array_free(regions, TRUE);
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
