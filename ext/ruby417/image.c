#include <stdlib.h>
#include "image.h"

struct image8 *image8_new_with_allocators(int width, int height,
                                          void *(*malloc)(size_t size),
                                          void (*free)(void *ptr)) {
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

struct image8 *image8_new(int width, int height) {
  return image8_new_with_allocators(width, height, malloc, free);
}

void image8_free(struct image8 *im) {
  if (im) {
    im->free(im->data);
    im->free(im);
  }
}

void image8_set(struct image8 *im, int x, int y, unsigned char val) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    im->data[im->width*y + x] = val;
  }
}

unsigned char image8_get(struct image8 *im, int x, int y) {
  return im->data[im->width*y + x];
}

unsigned char image8_get_with_fallback(struct image8 *im, int x, int y, unsigned char fallback) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    return image8_get(im, x, y);
  }
  return fallback;
}

struct image32 *image32_new_with_allocators(int width, int height,
                                            void *(*malloc)(size_t size),
                                            void (*free)(void *ptr)) {
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

struct image32 *image32_new(int width, int height) {
  return image32_new_with_allocators(width, height, malloc, free);
}

void image32_free(struct image32 *im) {
  if (im) {
    im->free(im->data);
    im->free(im);
  }
}

void image32_set(struct image32 *im, int x, int y, unsigned val) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    im->data[im->width*y + x] = val;
  }
}

unsigned image32_get(struct image32 *im, int x, int y) {
  return im->data[im->width*y + x];
}

unsigned image32_get_with_fallback(struct image32 *im, int x, int y, unsigned fallback) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    return image32_get(im, x, y);
  }
  return fallback;
}
