#include "rectangles.h"

/*
 * Records a connection (or, in this case, an equivilance) between two "sites,"
 * represented as integers. If the accumulator isn't large enough already, it
 * expands to fit the largest site.
 */
static int uf_union(DArray* acc, uint32_t a, uint32_t b)
{
  uint32_t max = (a > b) ? a : b;

  for (uint32_t z = acc->len; z <= max; z++) {
    if(!darray_push(acc, INT2PTR(z))) return 0;
  }

  darray_index_set(acc, uf_find(acc, a), INT2PTR(uf_find(acc, b)));

  return 1;
}

/*
 * Returns the root of the site, such that all connected sites share one root,
 * or -1 if the given site isn't recorded in the accumulator.
 * This implementation includes path compression for speed at little cost to
 * complexity.
 */
static int64_t uf_find(DArray* acc, uint32_t site)
{
  if (site < 0 || site >= acc->len) {
    return -1;
  } else if (darray_index(acc, site) == INT2PTR(site)) {
    return site;
  } else {
    /* The PTR2INT and INT2PTR conversions are messy, but they save a lot of
     * dynamic memory allocations. */
    return PTR2INT(darray_index_set(acc, site,
            INT2PTR(uf_find(acc, PTR2INT(darray_index(acc, site))))
           ));
  }
}

/*
 * Allocates and initializes an RDMatrix, or returns NULL if some memory
 * allocation failed. The contents of the matrix are initialized to zeros.
 */
static RDMatrix* rd_matrix_new(uint16_t width, uint16_t height)
{
  RDMatrix* m = malloc(sizeof(RDMatrix));

  if (m) {
    m->width = width;
    m->height = height;
    m->data = calloc(width*height, sizeof(m->data[0]));

    if(!m->data && width != 0 && height != 0) {
      free(m);
      return NULL;
    }
  }

  return m;
}

/*
 * Allocate and initialize an RDImage.
 */
static RDImage* rd_image_new(uint8_t* data, uint16_t width, uint16_t height)
{
  RDImage* i = malloc(sizeof(RDImage));

  if (i) {
    i->width = width;
    i->height = height;
    i->data = data;
  }

  return i;
}

/*
 * Given an RDImage, returns an RDMatrix of the same size that labels each image
 * pixel with a component number--pixels of the same label belong to the same
 * component. This is very helpful during contour tracing.
 *
 * The algorithm takes each pixel row by row, column by column, and, if there
 * are any labeled neighboring pixels of the same color, assignes that label to
 * the pixel. If not, the pixel is assigned a new, unique label. This method
 * often produces components that have different labels assigned to its pixels,
 * so Union-Find is used to track and resolve these equivalencies.
 */
static RDMatrix* rd_label_image_regions(RDImage* image)
{
  RDMatrix* labels = rd_matrix_new(image->width, image->height);
  DArray* label_eqvs = darray_new(128);
  if (!labels || !label_eqvs) goto nomem;

  uint32_t x, y, pixel_label, current_label = 1;
  for (y = 0; y < image->height; y++) {
    for (x = 0; x < image->width; x++) {
      pixel_label = rd_determine_label(image, labels, label_eqvs, x, y);

      if (pixel_label) {
        rd_matrix_set(labels, x, y, pixel_label);
      } else {
        rd_matrix_set(labels, x, y, current_label);
        /* expand the accumulator */
        if (!uf_union(label_eqvs, current_label, current_label)) {
          goto nomem;
        }

        current_label++;
      }
    }
  }

  /* Resolve label equivalencies by relabeling each pixel with the root label */
  for (uint32_t z = 0; z < image->width*image->height; z++) {
    labels->data[z] = uf_find(label_eqvs, labels->data[z]);
  }

  darray_free(label_eqvs, NULL);
  return labels;

nomem:
  /* if a memory allocation fails, set the error, free as much as possible, and
   * exit with NULL. */
  rd_error = RDNOMEM;
  darray_free(label_eqvs, NULL);
  rd_matrix_free(labels);
  return NULL;
}

/*
 * Examine the neighboring pixels in order to determine whether to assign an
 * existing component label, or to use a new one. Pixels are considered
 * neighbors according to 4-connectivity -- north/east/south/west.
 * 8-connectivity wouldn't be significantly different in implementation, but the
 * contour tracing algorithm used in rd_extract_contour() only works reliably
 * for 4-connected components.
 *
 * While examining the neighboring pixels labels, label equivalencies are also
 * established.
 */
static uint32_t rd_determine_label(RDImage* image, RDMatrix* labels, DArray* eqvs, uint16_t x, uint16_t y)
{
  static const int offsets[2][2] = {{-1, 0}, {0, -1}}; /* 4-connectivity */

  uint8_t color = rd_matrix_read_fast(image, x, y);
  int64_t nx, ny, neighbor_label, best_label = 0;

  for (int i = 0; i < 2; i++) {
    nx = offsets[i][0] + x;
    ny = offsets[i][1] + y;

    if (color == rd_matrix_read_safe(image, nx, ny, ~color)) {
      neighbor_label = rd_matrix_read_fast(labels, nx, ny);

      /* record equivalencies */
      if (best_label) uf_union(eqvs, best_label, neighbor_label);
      /* choose the lower label */
      if (!best_label || neighbor_label < best_label) best_label = neighbor_label;
    }
  }

  return best_label;
}

/*
 * Allocate and initialize an RDRegion, or return NULL if the allocation fails.
 */
static RDRegion* rd_region_new(void)
{
  RDRegion* region = malloc(sizeof(RDRegion));

  if (region) {
    region->boundary = darray_new(8);
    region->cx = 0;
    region->cy = 0;
    region->area = 0;

    if (!region->boundary) {
      free(region);
      return NULL;
    }
  }

  return region;
}

static void rd_region_free(RDRegion* region)
{
  if (region) {
    darray_free(region->boundary, (DArrayFreeFunc) free);
    free(region);
  }
}

static RDPoint* rd_point_new(uint16_t x, uint16_t y)
{
  RDPoint* point = malloc(sizeof(RDPoint));

  if (point) {
    point->x = x;
    point->y = y;
  }

  return point;
}

/*
 * Given an RDImage, calculate the component labels and extract certain region
 * information: region center, area, and the *outer* boundary pixels. To improve
 * performance, only pixels of intensities larger or equal to the threshold are
 * considered for components.
 */
static DArray* rd_extract_regions(RDImage* image, uint8_t intensity_threshold)
{
  DArray* regions = darray_new(16);
  RDMatrix* labels = rd_label_image_regions(image);
  if (!labels || !regions) goto nomem;

  uint16_t x, y;
  uint32_t label;
  RDRegion* region;

  for (y = 0; y < image->height; y++) {
    for (x = 0; x < image->width; x++) {
      if (rd_matrix_read_fast(image, x, y) >= intensity_threshold) {
        label = rd_matrix_read_fast(labels, x, y);
        region = darray_index(regions, label);

        if (!region) {
          /* create a new region and add it to the lookup */

          while(regions->len <= label) {
            /* resizing the dynamic array may fail */
            if(!darray_push(regions, NULL)) goto nomem;
          }

          region = rd_region_new();

          if (region) {
            rd_extract_contour(region->boundary, labels, x, y);

            /* populating the boundary array may fail */
            if (rd_error != ALLWELL) goto abort;

            darray_index_set(regions, label, region);
          } else {
            goto nomem;
          }
        }

        region->area++;
        region->cx += x;
        region->cy += y;
      }
    }
  }

  /* drop NULL elements (created during array expansion) and normalize region
   * center coordinates. */
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

  rd_matrix_free(labels);
  return regions;

nomem:
  rd_error = RDNOMEM;

abort:
  rd_matrix_free(labels);
  darray_free(regions, (DArrayFreeFunc) rd_region_free);
  return NULL;
}

/*
 * Beginning at the given pixel, the square-tracing algorithm is used to follow
 * the boundary of the component associated with that pixel clockwise. This
 * algorithm is simple and fast, but it only works reliably for 4-connected
 * components.
 */
static void rd_extract_contour(DArray* boundary, RDMatrix* labels, uint16_t start_x, uint16_t start_y)
{
  const int RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3;

  RDPoint* point;
  uint32_t label, target_label = rd_matrix_read_fast(labels, start_x, start_y);
  int direction = DOWN;
  int32_t x = start_x, y = start_y;

  do
    {
      label = rd_matrix_read_safe(labels, x, y, ~target_label);

      if (label == target_label) {
        if (boundary->len == 0 || x != point->x || y != point->y) {
          point = rd_point_new(x, y);

          if (!point || !darray_push(boundary, point)) {
            rd_error = RDNOMEM;
            return;
          }
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

/*
 * Implements the Graham Scan technique to find the convex hull of the given
 * region boundary.
 * Note: The resulting DArray is populated by pointers to RDPoints that are also
 * pointed to by the boundary DArray. Use rd_region_free() to free the region
 * and the memory pointed at by the boundary array, and darray_free(_, NULL) to
 * free the hull array in order to avoid double-free's.
 */
static DArray* rd_convex_hull(DArray* boundary)
{
  if(boundary->len < 3) return NULL;

  int status = 0;
  DArray* hull = darray_new(boundary->len / 4);
  if(!hull) goto nomem;

  status = darray_push(hull, darray_index(boundary, 0));
  if (status != 1) goto nomem;

  status = darray_msort(boundary, darray_index(boundary, 0), (DArrayCompareFunc) rd_graham_cmp);
  if (status != 1) goto nomem;

  uint32_t p = 1;

  while (p <= boundary->len) {
    if (hull->len == 1
        || rd_vector_cross(darray_index(hull, hull->len-2),
                           darray_index(hull, hull->len-1),
                           darray_index(hull, hull->len-1),
                           darray_index(boundary, p % boundary->len)) > 0) {
      if(++p <= boundary->len) {
        if (darray_push(hull, darray_index(boundary, p-1)) != 1)
          goto nomem;
      }
    } else
      darray_remove_fast(hull, hull->len-1);
  }

  return hull;

nomem:

  rd_error = RDNOMEM;
  darray_free(hull, NULL);
  return NULL;
}

/*
 * Returns an integer with the same sign as the angle between base->p vector and
 * base->q vector.
 */
static int rd_graham_cmp(RDPoint* p, RDPoint* q, RDPoint* base)
{
  return -rd_vector_cross(base, p, base, q);
}

/*
 * Returns the vector dot product of a->b and c->d, which is proportional to the
 * cosine of the angle between them.
 */
static int32_t rd_vector_dot(RDPoint* a, RDPoint* b, RDPoint* c, RDPoint* d)
{
  return (b->x-a->x)*(d->x-c->x) + (b->y-a->y)*(d->y-c->y);
}

/*
 * Returns the z-component magnitude of the vector cross product of a->b and
 * c->d, which is proportional to the sine of the angle between them.
 */
static int32_t rd_vector_cross(RDPoint* a, RDPoint* b, RDPoint* c, RDPoint* d)
{
  return (b->x-a->x)*(d->y-c->y) - (b->y-a->y)*(d->x-c->x);
}

/*
 * Given a convex hull, determines the minimum-area rectangle that fits the
 * hull. The main trick, proved by Freeman and Shapira in 1975, is that one edge
 * of the rectangle must be collinear with one edge of the hull.
 *
 * The algorithm used here is described in "A Linear Time Algorithm for the
 * Minimum Area Rectangle Enclosing a Convex Polygon" (Arnon and Gieselmann,
 * 1983).
 */
static RDRectangle* rd_fit_rectangle(DArray* hull)
{
  RDRectangle* rectangle = malloc(sizeof(RDRectangle));

  if (rectangle) {
    double min_area = -1, slope, width, height;
    uint32_t base_idx = 0, leftmost_idx = base_idx, altitude_idx = 0, rightmost_idx = 0;
    RDPoint* left_base_point, *right_base_point;

    while (base_idx < hull->len)
    {
      /* consider one edge of the hull */
      left_base_point  = rd_hull_wrap_index(hull, base_idx);
      right_base_point = rd_hull_wrap_index(hull, base_idx+1);

      /* find the point on the hull that is furthest to the left from the edge
         (relative to the edge, which begins at the highest point and runs
         clockwise). */
      while (rd_vector_dot(left_base_point,
                           right_base_point,
                           rd_hull_wrap_index(hull, leftmost_idx),
                           rd_hull_wrap_index(hull, leftmost_idx+1)) > 0)
      {
        ++leftmost_idx;
      }

      /* Find the point furthest from the edge in the perpendicular direction.
        This will be at least equal to the leftmost point, generally past. */
      if(altitude_idx == 0) altitude_idx = leftmost_idx;

      while (rd_vector_cross(left_base_point,
                             right_base_point,
                             rd_hull_wrap_index(hull, altitude_idx),
                             rd_hull_wrap_index(hull, altitude_idx+1)) > 0)
      {
        ++altitude_idx;
      }

      /* Find the point that is furthest to the right from the edge, relative to
         the edge. */
      if(rightmost_idx == 0) rightmost_idx = altitude_idx;

      while (rd_vector_dot(left_base_point,
                           right_base_point,
                           rd_hull_wrap_index(hull, rightmost_idx),
                           rd_hull_wrap_index(hull, rightmost_idx+1)) < 0)
      {
        ++rightmost_idx;
      }

      /* Determine the dimensions described by these points. If the edge is
         vertical or horizontal, the scenario must be handled separately to
         avoid division issues.

         The +1's are needed, I think, to counter the fencepost issue between
         discrete pixels and the true geometric distance. They are necessary to
         to produce he correct answer, anyway. TODO. */
      if (left_base_point->x == right_base_point->x) {
        width  = abs(rd_hull_wrap_index(hull, rightmost_idx)->y - rd_hull_wrap_index(hull, leftmost_idx)->y)+1;
        height = abs(rd_hull_wrap_index(hull, altitude_idx)->x - left_base_point->x)+1;
      } else if (left_base_point->y == right_base_point->y) {
        width  = abs(rd_hull_wrap_index(hull, rightmost_idx)->x - rd_hull_wrap_index(hull, leftmost_idx)->x)+1;
        height = abs(rd_hull_wrap_index(hull, altitude_idx)->y - left_base_point->y)+1;
      } else {
        slope = (double) (left_base_point->y - right_base_point->y) / (left_base_point->x - right_base_point->x);
        width = rd_line_distance(rd_hull_wrap_index(hull, leftmost_idx), rd_hull_wrap_index(hull, rightmost_idx), -1/slope)+1;
        height = rd_line_distance(left_base_point, rd_hull_wrap_index(hull, altitude_idx), slope)+1;
      }

      if (width*height < min_area || min_area < 0) {
        RDPoint upper_left, upper_right;
        int32_t dx = right_base_point->x - left_base_point->x,
                dy = right_base_point->y - left_base_point->y;

        /* determine the rectangle orientation */
        if (dx == 0) rectangle->orientation = M_PI_2;
        else         rectangle->orientation = atan((double) dy/dx);

        /* this adjustment is made necessary by the fact that the sign of the
           angle is lost during division, and consequently by atan() */
        if (dx < 0 || (dx == 0 && dy < 0)) rectangle->orientation += M_PI;

        /* find two corner pixels along the base side of the rectangle, and use
           them to determine the rectangle's center. */
        rd_determine_fourth_point(left_base_point, right_base_point, rd_hull_wrap_index(hull, leftmost_idx), &upper_left);
        rd_determine_fourth_point(left_base_point, right_base_point, rd_hull_wrap_index(hull, rightmost_idx), &upper_right);
        rectangle->cx = (uint16_t) (upper_left.x + upper_right.x - sin(rectangle->orientation)*height) / 2;
        rectangle->cy = (uint16_t) (upper_left.y + upper_right.y + cos(rectangle->orientation)*height) / 2;

        rectangle->width = (uint16_t) width;
        rectangle->height = (uint16_t) height;

        min_area = width*height;
      }

      ++base_idx;
    }
  } else {
    rd_error = RDNOMEM;
  }

  return rectangle;
}

static RDPoint* rd_hull_wrap_index(DArray* hull, uint32_t index)
{
  return darray_index(hull, index % hull->len);
}

/*
 * Returns the perpendicular distance between a line of slope slope through the
 * point p and point q.
 */
static double rd_line_distance(RDPoint* p, RDPoint* q, double slope)
{
  return abs((p->y - slope*p->x) - (q->y - slope*q->x)) / sqrt(pow(slope, 2) + 1);
}

/*
 * Given two points, p1 and p2, on the same line, and a third point, q1 that
 * lies on another line perpendicular to the first, determines the intersection
 * point of the lines, q2.
 */
static void rd_determine_fourth_point(RDPoint* p1, RDPoint* p2, RDPoint* q1, RDPoint* q2)
{
  if (p1->x == p2->x) {
    q2->x = p1->x;
    q2->y = q1->y;
  } else {
    double slope = (double) (p2->y - p1->y) / (p2->x - p1->x);
    double x = (double) (slope*(q1->y-p1->y) + q1->x + p1->x*pow(slope, 2)) / (pow(slope, 2) + 1);

    q2->x = (uint16_t) x;
    q2->y = (uint16_t) (slope*(x - p1->x)) + p1->y;
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
  /* reset error code */
  rd_error = ALLWELL;

  VALUE rect_data = rb_ary_new();
  RDImage* image = NULL;
  DArray* regions = NULL,
        * hull;
  RDRegion* region;

  image = rd_image_new(data, width, height);
  if(!image) rb_raise(rb_eNoMemError, "unable to allocate sufficient memory");

  regions = rd_extract_regions(image, intensity_threshold);
  if (rd_error != ALLWELL) goto upon_error;

  for (uint32_t r = 0; r < regions->len; r++) {
    region = darray_index(regions, r);

    if (region->area >= area_threshold && region->boundary->len >= 3) {
      hull = rd_convex_hull(region->boundary);
      if (rd_error != ALLWELL) goto upon_error;

      RDRectangle* rect = rd_fit_rectangle(hull);
      if (rd_error != ALLWELL) goto upon_error;

      VALUE rect_args[6] = { INT2FIX(rect->cx),    INT2FIX(rect->cy),
                             INT2FIX(rect->width), INT2FIX(rect->height),
                             rb_float_new(rect->orientation), INT2FIX(region->area)};
      VALUE rb_rect = rb_class_new_instance(6, rect_args, cRectangle);

      rb_ary_push(rect_data, rb_rect);

      darray_free(hull, NULL);
      free(rect);
    }
  }

  darray_free(regions, (DArrayFreeFunc) rd_region_free);
  free(image);

  return rect_data;

upon_error:

  if (hull) darray_free(hull, NULL);
  darray_free(regions, (DArrayFreeFunc) rd_region_free);
  free(image);

  switch(rd_error) {
    case RDNOMEM:
      rb_raise(rb_eNoMemError, "unable to allocate sufficient memory");
    default:
      rb_raise(rb_eRuntimeError, "BUG: an unexpected error occurred within Ruby417 native extension (code %i)", rd_error);
  }
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
