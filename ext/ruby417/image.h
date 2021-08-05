#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>

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

struct image8 *image8_new_with_allocators(int width, int height,
                                          void *(*malloc)(size_t size),
                                          void (*free)(void *ptr));
struct image8 *image8_new(int width, int height);
void image8_free(struct image8 *im);
void image8_set(struct image8 *im, int x, int y, unsigned char val);
unsigned char image8_get(struct image8 *im, int x, int y);
unsigned char image8_get_with_fallback(struct image8 *im, int x, int y, unsigned char fallback);

struct image32 *image32_new_with_allocators(int width, int height,
                                            void *(*malloc)(size_t size),
                                            void (*free)(void *ptr));
struct image32 *image32_new(int width, int height);
void image32_free(struct image32 *im);
void image32_set(struct image32 *im, int x, int y, unsigned val);
unsigned image32_get(struct image32 *im, int x, int y);
unsigned image32_get_with_fallback(struct image32 *im, int x, int y, unsigned fallback);

#endif
