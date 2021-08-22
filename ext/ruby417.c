#include "ruby417/darray.c"
#include "ruby417/image.c"
#include "ruby417/rectangles.c"

#ifdef BUILD_RUBY_EXT

#include <ruby.h>

static VALUE mRuby417, mExt;

#define ensure_float_percentage(val, name) \
  do { \
    if (val < 0 || val > 1) rb_raise(rb_eRangeError, name " should be between 0 and 1, got %f", val); \
  } while(0)

static VALUE locate_via_guards(VALUE self, VALUE im_data, VALUE width, VALUE height,
                               VALUE area_threshold, VALUE rectangularity_threshold,
                               VALUE angle_variation_threshold, VALUE area_variation_threshold,
                               VALUE width_variation_threshold, VALUE height_variation_threshold,
                               VALUE guard_aspect_min, VALUE guard_aspect_max,
                               VALUE barcode_aspect_min, VALUE barcode_aspect_max) {
  Check_Type(im_data, T_STRING);
  Check_Type(width, T_FIXNUM);
  Check_Type(height, T_FIXNUM);
  Check_Type(area_threshold, T_FIXNUM);
  Check_Type(rectangularity_threshold, T_FLOAT);
  Check_Type(angle_variation_threshold, T_FLOAT);
  Check_Type(area_variation_threshold, T_FLOAT);
  Check_Type(width_variation_threshold, T_FLOAT);
  Check_Type(height_variation_threshold, T_FLOAT);
  Check_Type(guard_aspect_min, T_FIXNUM);
  Check_Type(guard_aspect_max, T_FIXNUM);
  Check_Type(barcode_aspect_min, T_FIXNUM);
  Check_Type(barcode_aspect_max, T_FIXNUM);

  int c_width  = FIX2INT(width),
      c_height = FIX2INT(height),
      c_area_threshold = FIX2INT(area_threshold),
      c_guard_aspect_min = FIX2INT(guard_aspect_min),
      c_guard_aspect_max = FIX2INT(guard_aspect_max),
      c_barcode_aspect_min = FIX2INT(barcode_aspect_min),
      c_barcode_aspect_max = FIX2INT(barcode_aspect_max);
  double c_rectangularity_threshold = RFLOAT_VALUE(rectangularity_threshold),
         c_angle_variation_threshold = RFLOAT_VALUE(angle_variation_threshold),
         c_area_variation_threshold = RFLOAT_VALUE(area_variation_threshold),
         c_width_variation_threshold = RFLOAT_VALUE(width_variation_threshold),
         c_height_variation_threshold = RFLOAT_VALUE(height_variation_threshold);

  if (RSTRING_LEN(im_data) != c_width*c_height) {
    rb_raise(rb_eEOFError, "image data and dimensions (%ix%i) do not align", c_width, c_height);
  } else if (c_width < 0 || c_height < 0) {
    rb_raise(rb_eRangeError, "image dimensions are negative (%ix%i)", c_width, c_height);
  }
  ensure_float_percentage(c_rectangularity_threshold, "rectangularity threshold");
  ensure_float_percentage(c_area_variation_threshold, "area variation threshold");
  ensure_float_percentage(c_width_variation_threshold, "width variation threshold");
  ensure_float_percentage(c_height_variation_threshold, "height variation threshold");

  struct pairing_settings settings = {
    .area_threshold = c_area_threshold,
    .rectangularity_threshold = c_rectangularity_threshold,
    .angle_variation_threshold = c_angle_variation_threshold,
    .area_variation_threshold = c_area_variation_threshold,
    .width_variation_threshold = c_width_variation_threshold,
    .height_variation_threshold = c_height_variation_threshold,
    .guard_aspect_min = c_guard_aspect_min,
    .guard_aspect_max = c_guard_aspect_max,
    .barcode_aspect_min = c_barcode_aspect_min,
    .barcode_aspect_max = c_barcode_aspect_max
  };

  struct image8 image = {
    .width = c_width,
    .height = c_height,
    .free = NULL,
    .data = (unsigned char *) StringValuePtr(im_data)
  };

  VALUE located_barcodes = rb_ary_new();
  struct image32 *labeled = image_label_regions(&image, malloc, realloc, free);
  struct darray *regions = NULL, *hull = NULL, *rects = NULL, *pairs = NULL;
  struct rectangle *rect;

  if (!(regions=image_extract_regions(&image, labeled, malloc, realloc, free)) ||
      !(hull=darray_new(0, NULL, malloc, realloc, free)) ||
      !(rects=darray_new(0, free, malloc, realloc, free)) ||
      !(pairs=darray_new(0, free, malloc, realloc, free))) goto oom;

  for (unsigned i = 0; i < regions->len; i++) {
    struct region *region = darray_index(regions, i);

    if (region->area >= c_area_threshold && region->boundary->len > 2) {
      if (!boundary_convex_hull(region->boundary, hull)) goto oom;
      if (hull->len > 2) {
        if (!(rect = malloc(sizeof(*rect)))) goto oom;
        hull_minimal_rectangle(hull, region->area, rect);
        if (!darray_push(rects, rect)) {
          free(rect);
          goto oom;
        }
      }
      hull->len = 0; // reset for reuse, no freeing necessary
    }
  }

  if (!pair_aligned_rectangles(&settings, rects, pairs)) goto oom;

  for (unsigned i = 0; i < pairs->len; i++) {
    struct rectangle_pair *pair = darray_index(pairs, i);
    struct barcode_corners corners;
    determine_barcode_corners(pair, &corners);
    VALUE barcode_data = rb_ary_new_from_args(9, DBL2NUM(pair->score),
                                                 INT2FIX(corners.upper_left.x), INT2FIX(corners.upper_left.y),
                                                 INT2FIX(corners.lower_left.x), INT2FIX(corners.lower_left.y),
                                                 INT2FIX(corners.lower_right.x), INT2FIX(corners.lower_right.y),
                                                 INT2FIX(corners.upper_right.x), INT2FIX(corners.upper_right.y));
    rb_ary_push(located_barcodes, barcode_data);
  }

  image32_free(labeled);
  darray_free(regions, true);
  darray_free(hull, false);
  darray_free(rects, true);
  darray_free(pairs, true);
  return located_barcodes;

oom:
  image32_free(labeled);
  darray_free(regions, true);
  darray_free(hull, false);
  darray_free(rects, true);
  darray_free(pairs, true);
  rb_raise(rb_eNoMemError, "unable to allocate sufficient memory");
}

void Init_ruby417(void) {
  mRuby417 = rb_define_module("Ruby417");
  mExt = rb_define_module_under(mRuby417, "Ext");

  rb_define_module_function(mExt, "locate_via_guards", locate_via_guards, 13);
}

#endif
