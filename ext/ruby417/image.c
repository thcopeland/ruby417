#include <stdlib.h>
#include <string.h> /* memcpy */
#include "image.h"

struct image *image_new_with_allocators(int width, int height, size_t eltsize,
                                        void *(*malloc)(size_t size),
                                        void (*free)(void *ptr)) {
  struct image *im = malloc(sizeof(*im));

  if (im) {
    im->width = width;
    im->height = height;
    im->eltsize = eltsize;
    im->free = free;
    im->data = malloc(eltsize*width*height);

    if (!im->data) {
      free(im);
      return NULL;
    }
  }

  return im;
}

struct image *image_new(int width, int height, size_t eltsize) {
  return image_new_with_allocators(width, height, eltsize, malloc, free);
}

void image_free(struct image *im) {
  if (im) {
    im->free(im->data);
    im->free(im);
  }
}

void image_set(struct image *im, int x, int y, void *val) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    memcpy(image_get(im, x, y), val, im->eltsize);
  }
}

void *image_get(struct image *im, int x, int y) {
  return im->data + im->eltsize*(im->width*y + x);
}

void *image_get_with_fallback(struct image *im, int x, int y, void *fallback) {
  if (x >= 0 && x < im->width && y >= 0 && y < im->height) {
    return image_get(im, x, y);
  }
  return fallback;
}
