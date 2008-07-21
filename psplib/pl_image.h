/* psplib/pl_image.h
   Image manipulation/saving/loading

   Copyright (C) 2007-2008 Akop Karapetyan

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Author contact information: pspdev@akop.org
*/

#ifndef _PL_IMAGE_H
#define _PL_IMAGE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PL_IMAGE_USE_VRAM  0x01

typedef unsigned int uint;

typedef enum pl_image_format_t
{
  pl_image_indexed = 1,
  pl_image_4444,
  pl_image_5551
} pl_image_format;

typedef struct pl_image_palette_t
{
  void *palette;
  uint  colors;
  pl_image_format
        format;
} pl_image_palette;

typedef struct pl_image_view_t
{
  uint x;
  uint y;
  uint w;
  uint h;
} pl_image_view;

typedef struct pl_image_t
{
  pl_image_view
          view;
  uint    height;
  uint    line_width;
  uint    pitch;
  pl_image_format
          format;
  pl_image_palette
          palette;
  uint8_t flags;
  void   *bitmap;
} pl_image;

int  pl_image_create(pl_image *image,
                     uint width,
                     uint height,
                     pl_image_format format,
                     uint8_t flags);
void pl_image_destroy(pl_image *image);
int  pl_image_create_duplicate(const pl_image *original,
                               pl_image *copy);
int  pl_image_palettize(pl_image *image,
                        pl_image_format pal_format,
                        uint pal_colors);
int  pl_image_set_palette_color(pl_image *image,
                                uint index,
                                uint32_t color);
uint pl_image_get_depth(const pl_image *image);
uint pl_image_get_bytes_per_pixel(pl_image_format format);
uint32_t
     pl_image_get_composite_color(pl_image_format dest_format,
                                  uint8_t red,
                                  uint8_t green,
                                  uint8_t blue,
                                  uint8_t alpha);
void pl_image_split_color(pl_image_format src_format,
                          uint32_t color,
                          uint8_t *red,
                          uint8_t *green,
                          uint8_t *blue,
                          uint8_t *alpha);
int  pl_image_load_png_stream(pl_image *image,
                              FILE *stream);
int  pl_image_save_png_stream(const pl_image *image,
                              FILE *stream);
int  pl_image_load_png(pl_image *image,
                       const char *path);
int  pl_image_save_png(const pl_image *image,
                       const char *path);
int  pl_image_create_thumbnail(const pl_image *original,
                               pl_image *thumb);

/*
void pl_image_clear(pl_image *image,
                    uint32_t color);
*/
/* Create/destroy *

PspImage* pspImageRotate(const PspImage *orig, int angle_cw);
PspImage* pspImageCreateThumbnail(const PspImage *image);
void      pspImageClear(PspImage *image, unsigned int color);

PspImage* pspImageLoadPng(const char *path);
int       pspImageSavePng(const char *path, const PspImage* image);
PspImage* pspImageLoadPngFd(FILE *fp);
int       pspImageSavePngFd(FILE *fp, const PspImage* image);

int pspImageBlur(const PspImage *original, PspImage *blurred);
int pspImageDiscardColors(const PspImage *original);
*/

#ifdef __cplusplus
}
#endif

#endif  // _PL_IMAGE_H