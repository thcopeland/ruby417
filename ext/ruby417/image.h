#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h> // size_t

struct image8 {
  int width;
  int height;
  void (*free)(void *ptr);
  unsigned char *data;
};

struct image32 {
  int width;
  int height;
  void (*free)(void *ptr);
  unsigned *data;
};

struct point {
  int x, y;
};

struct region {
  struct darray *boundary;
  long cx, cy;
  long area;
};


static struct image8 *image8_new(int width, int height, void *(*malloc)(size_t size), void (*free)(void *ptr));
static void image8_free(struct image8 *im);
static void image8_set(struct image8 *im, int x, int y, unsigned char val);
static unsigned char image8_get(struct image8 *im, int x, int y);
static unsigned char image8_get_with_fallback(struct image8 *im, int x, int y, unsigned char fallback);

static struct image32 *image32_new(int width, int height, void *(*malloc)(size_t size), void (*free)(void *ptr));
static void image32_free(struct image32 *im);
static void image32_set(struct image32 *im, int x, int y, unsigned val);
static unsigned image32_get(struct image32 *im, int x, int y);
static unsigned image32_get_with_fallback(struct image32 *im, int x, int y, unsigned fallback);

static struct point *point_new(int x, int y, void *(*malloc)(size_t size));
static void point_rotate(struct point *p, struct point *origin, double angle, struct point *out);
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

#endif
