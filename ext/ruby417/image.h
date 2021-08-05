#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>

struct image {
  int width;
  int height;
  size_t eltsize;
  void (*free)(void *ptr);
  unsigned char *data;
};

struct image *image_new_with_allocators(int width, int height, size_t eltsize,
                                        void *(*malloc)(size_t size),
                                        void (*free)(void *ptr));
struct image *image_new(int width, int height, size_t eltsize);
void image_free(struct image *im);
void image_set(struct image *im, int x, int y, void *val);
void *image_get(struct image *im, int x, int y);
void *image_get_with_fallback(struct image *im, int x, int y, void *fallback);

#endif
