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

#include <malloc.h>
#include <string.h>
#include <png.h>

#include "video.h" /* TODO: */
#include "pl_image.h"
#include "pl_file.h"

static uint get_next_power_of_two(uint n);
static uint get_bitmap_size(const pl_image *image);
int copy_from_void(pl_image_format format,
                   void *from,
                   uint32_t *to);
int copy_to_void(pl_image_format format,
                 uint32_t from,
                 void *to);

int pl_image_create(pl_image *image,
                    uint width,
                    uint height,
                    pl_image_format format,
                    uint8_t flags)
{
  uint bytes_per_pixel = pl_image_get_bytes_per_pixel(format);
  if (!bytes_per_pixel)
    return 0;

  /* Allocate memory */
  uint line_width = get_next_power_of_two(width);
  uint pitch = line_width * bytes_per_pixel;
  uint buf_len = pitch * height;
  void *buffer = NULL;

  if (flags & PL_IMAGE_USE_VRAM) /* use VRAM */
    buffer = pspVideoAllocateVramChunk(buf_len);
  else   /* use heap */
    buffer = memalign(16, buf_len);

  if (!buffer) return 0;

  memset(buffer, 0, buf_len);

  /* Init. structure */
  image->view.x = image->view.y = 0;
  image->view.w = width;
  image->view.h = height;
  image->height = height;
  image->line_width = line_width;
  image->pitch  = pitch;
  image->format = format;
  image->flags  = flags;
  image->bitmap = buffer;

  image->palette.palette = NULL;
  image->palette.colors = 0;
  image->palette.format = 0;

  return 1;
}

void pl_image_destroy(pl_image *image)
{
  /* Release bitmap */
  if (!(image->flags & PL_IMAGE_USE_VRAM))
    free(image->bitmap);

  /* Release palette */
  if (image->palette.palette)
    free(image->palette.palette);
}

int pl_image_palettize(pl_image *image,
                       pl_image_format pal_format,
                       uint pal_colors)
{
  if ((image->format != pl_image_indexed) || /* palette only valid for indexed images */
      (pal_colors == 0) ||              /* need at least 1 color */
      (pal_colors & 15) ||              /* number of colors must be divisible by 16 */
      (pal_format == pl_image_indexed)) /* 'indexed' not valid for palette format */
    return 0;

  /* Allocate pal. space */
  uint bytes_per_pixel = pl_image_get_bytes_per_pixel(pal_format);
  void *palette;
  if (!(palette = memalign(16, bytes_per_pixel * pal_colors)))
    return 0;

  /* Release current pal. */
  if (image->palette.palette)
    free(image->palette.palette);

  /* Reset image pal. */
  image->palette.format  = pal_format;
  image->palette.colors  = pal_colors;
  image->palette.palette = palette;

  return 1;
}

int pl_image_set_palette_color(pl_image *image,
                               uint index,
                               uint32_t color)
{
  if (image->format != pl_image_indexed ||
      index > image->palette.colors)
    return 0;

  switch (image->palette.format)
  {
  case pl_image_4444:
  case pl_image_5551:
    ((uint16_t*)(image->palette.palette))[index] = color & 0xffff;
    return 1;
  default:
    return 0;
  }
}

int pl_image_create_duplicate(const pl_image *original,
                              pl_image *copy)
{
  /* create image */
  if (!pl_image_create(copy,
                       original->line_width,
                       original->height,
                       original->format,
                       0)) /* TODO: all but vram flag */
    return 0;

  /* copy palette */
  if (original->palette.palette)
  {
    if (!pl_image_palettize(copy,
                            original->palette.format,
                            original->palette.colors))
    {
      pl_image_destroy(copy);
      return 0;
    }

    uint pal_size = copy->palette.colors *
                    pl_image_get_bytes_per_pixel(copy->palette.format);
    memcpy(copy->palette.palette,
           original->palette.palette,
           pal_size);
  }

  /* copy image */
  memcpy(copy->bitmap,
         original->bitmap,
         get_bitmap_size(copy));

  /* copy misc. attributes */
  copy->view = original->view;

  return 1;
}

uint pl_image_get_bytes_per_pixel(pl_image_format format)
{
  switch (format)
  {
  case pl_image_indexed:
    return 1;
  case pl_image_4444:
  case pl_image_5551:
    return 2;
  default:
    return 0;
  }
}

uint pl_image_get_depth(const pl_image *image)
{
  return pl_image_get_bytes_per_pixel(image->format) << 3;
}

int pl_image_compose_color(pl_image_format dest_format,
                           uint32_t *color,
                           uint8_t red,
                           uint8_t green,
                           uint8_t blue,
                           uint8_t alpha)
{
  switch (dest_format)
  {
  case pl_image_4444:
    *color = (((alpha >> 4) & 0x0F) << 12) |
             (((blue  >> 4) & 0x0F) << 8) |
             (((green >> 4) & 0x0F) << 4) |
                ((red >> 4) & 0x0F);
    return 1;
  case pl_image_5551:
    *color = (((alpha >> 7) & 0x01) << 15) |
             (((blue  >> 3) & 0x1F) << 10) |
             (((green >> 3) & 0x1F) << 5) |
                ((red >> 3) & 0x1F);
    return 1;
  default:
    return 0;
  }
}

int pl_image_split_color(pl_image_format src_format,
                         const pl_image_palette *src_palette,
                         uint32_t color,
                         uint8_t *red,
                         uint8_t *green,
                         uint8_t *blue,
                         uint8_t *alpha)
{
  switch (src_format)
  {
  case pl_image_indexed:
    /* Get actual color */
    copy_from_void(src_palette->format,
                   src_palette->palette + color *
                     pl_image_get_bytes_per_pixel(src_palette->format),
                   &color);

    /* Split color */
    return pl_image_split_color(src_palette->format,
                                NULL,
                                color,
                                red,
                                green,
                                blue,
                                alpha);
  case pl_image_4444:
    *red   = (color & 0x0F) * 0xFF/0x0F;
    *green = ((color >> 4) & 0x0F) * 0xFF/0x0F;
    *blue  = ((color >> 8) & 0x0F) * 0xFF/0x0F;
    *alpha = ((color >> 12) & 0x0F) * 0xFF/0x0F;
    return 1;
  case pl_image_5551:
    *red   = (color & 0x1F) * 0xFF/0x1F;
    *green = ((color >> 5) & 0x1F) * 0xFF/0x1F;
    *blue  = ((color >> 10) & 0x1F) * 0xFF/0x1F;
    *alpha = ((color >> 15) & 0x01) * 0xFF/0x01;
    return 1;
  default:
    return 0;
  }
}

int pl_image_load_png_stream(pl_image *image,
                             FILE *stream)
{
  /* hardcoded for now */
  pl_image_format format = pl_image_5551;

  const size_t nSigSize = 8;
  uint8_t signature[nSigSize];
  if (fread(signature, sizeof(uint8_t), nSigSize, stream) != nSigSize)
    return 0;

  if (!png_check_sig(signature, nSigSize))
    return 0;

  png_struct *pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!pPngStruct)
    return 0;

  png_info *pPngInfo = png_create_info_struct(pPngStruct);
  if(!pPngInfo)
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  if (setjmp(pPngStruct->jmpbuf))
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  png_init_io(pPngStruct, stream);
  png_set_sig_bytes(pPngStruct, nSigSize);
  png_read_png(pPngStruct, pPngInfo,
    PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
    PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR , NULL);

  png_uint_32 width = pPngInfo->width;
  png_uint_32 height = pPngInfo->height;
  int color_type = pPngInfo->color_type;

  if (!pl_image_create(image,
                       width,
                       height,
                       format,
                       0))
  {
    png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
    return 0;
  }

  png_byte **pRowTable = pPngInfo->row_pointers;
  unsigned int x, y;
  uint8_t r, g, b, a;
  uint32_t color;
  int bytes_per_pixel = pl_image_get_bytes_per_pixel(format);
  void *line_ptr, *pel_ptr;

  for (y = 0, line_ptr = image->bitmap;
       y < height;
       y++, line_ptr += image->pitch)
  {
    png_byte *pRow = pRowTable[y];

    for (x = 0, pel_ptr = line_ptr;
         x < width;
         x++, pel_ptr += bytes_per_pixel)
    {
      switch(color_type)
      {
        case PNG_COLOR_TYPE_GRAY:
          r = g = b = *pRow++;
          a = 0xff;
          break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
          r = g = b = pRow[0];
          a = pRow[1];
          pRow += 2;
          break;
        case PNG_COLOR_TYPE_RGB:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          a = 0xff;
          pRow += 3;
          break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          a = pRow[3];
          pRow += 4;
          break;
        default:
          r = g = b = a = 0;
          break;
      }

      pl_image_compose_color(format, &color, r, g, b, a);
      copy_to_void(format, color, pel_ptr);
    }
  }

  png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);

  return 1;
}

int pl_image_save_png_stream(const pl_image *image,
                             FILE *stream)
{
  unsigned char *bitmap;
  int i, j;
  uint8_t r, g, b, a;
  int width = image->view.w;
  int height = image->view.h;
  int bytes_per_pixel = pl_image_get_bytes_per_pixel(image->format);
  void *line_ptr, *pel_ptr;
  uint32_t color;

  if (!(bitmap = (u8*)malloc(sizeof(u8) * width * height * 3)))
    return 0;

  for (i = 0, line_ptr = image->bitmap + (image->view.y * image->pitch);
       i < height;
       i++, line_ptr += image->pitch)
  {
    /* Skip to the start of the viewport */
    for (j = 0, pel_ptr = line_ptr + (image->view.x * bytes_per_pixel);
         j < width;
         j++, pel_ptr += bytes_per_pixel)
    {
      copy_from_void(image->format, pel_ptr, &color);
      pl_image_split_color(image->format,
                           &image->palette,
                           color,
                           &r,&g,&b,&a);

      bitmap[i * width * 3 + j * 3 + 0] = r;
      bitmap[i * width * 3 + j * 3 + 1] = g;
      bitmap[i * width * 3 + j * 3 + 2] = b;
    }
  }

  png_struct *pPngStruct = png_create_write_struct( PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL );

  if (!pPngStruct)
  {
    free(bitmap);
    return 0;
  }

  png_info *pPngInfo = png_create_info_struct( pPngStruct );
  if (!pPngInfo)
  {
    png_destroy_write_struct( &pPngStruct, NULL );
    free(bitmap);
    return 0;
  }

  png_byte **buf = (png_byte**)malloc(height * sizeof(png_byte*));
  if (!buf)
  {
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    free(bitmap);
    return 0;
  }

  unsigned int y;
  for (y = 0; y < height; y++)
    buf[y] = (uint8_t*)&bitmap[y * width * 3];

  if (setjmp( pPngStruct->jmpbuf ))
  {
    free(buf);
    free(bitmap);
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    return 0;
  }

  png_init_io( pPngStruct, stream );
  png_set_IHDR( pPngStruct, pPngInfo, width, height, 8,
    PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT);
  png_write_info( pPngStruct, pPngInfo );
  png_write_image( pPngStruct, buf );
  png_write_end( pPngStruct, pPngInfo );

  png_destroy_write_struct( &pPngStruct, &pPngInfo );
  free(buf);
  free(bitmap);

  return 1;
}

int pl_image_load(pl_image *image,
                  const char *path)
{
  int status = 0;
  FILE *stream = NULL;

  if (pl_file_is_of_type(path, "png"))
  {
    if ((stream = fopen(path, "r")))
      status = pl_image_load_png_stream(image,
                                        stream);
  }
  else status = 0;

  if (stream) fclose(stream);
  return status;
}

int pl_image_save(const pl_image *image,
                  const char *path)
{
  int status = 0;
  FILE *stream = NULL;

  if (pl_file_is_of_type(path, "png"))
  {
    if ((stream = fopen(path, "w")))
      status = pl_image_save_png_stream(image,
                                        stream);
  }
  else status = 0;

  if (stream) fclose(stream);
  return status;
}

/*
  PspImage *thumb;
  int i, j, p;

  if (!(thumb = pspImageCreate(image->Viewport.Width >> 1,
    image->Viewport.Height >> 1, image->Depth)))
      return NULL;

  int dy = image->Viewport.Y + image->Viewport.Height;
  int dx = image->Viewport.X + image->Viewport.Width;

  for (i = image->Viewport.Y, p = 0; i < dy; i += 2)
    for (j = image->Viewport.X; j < dx; j += 2)
      if (image->Depth == PSP_IMAGE_INDEXED)
        ((unsigned char*)thumb->Pixels)[p++]
          = ((unsigned char*)image->Pixels)[(image->Width * i) + j];
      else
        ((unsigned short*)thumb->Pixels)[p++]
          = ((unsigned short*)image->Pixels)[(image->Width * i) + j];

  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    memcpy(thumb->Palette, image->Palette, sizeof(image->Palette));
    thumb->PalSize = image->PalSize;
  }

  return thumb;
*/
int pl_image_create_thumbnail(const pl_image *original,
                              pl_image *thumb)
{
return pl_image_create_duplicate(original, thumb);
#if 0
  /* create image */
  if (!pl_image_create(thumb,
                       original->view.w / 2,
                       original->view.h / 2,
                       original->format,
                       0)) /* TODO: all but vram flag */
    return 0;

  int dy = original->view.y + original->view.h;
  int dx = original->view.x + original->view.w;

  for (i = original->view.y, p = 0; i < dy; i += 2)
    for (j = original->view.x; j < dx; j += 2)
    {
    }
      if (original->Depth == PSP_IMAGE_INDEXED)
        ((unsigned char*)thumb->Pixels)[p++]
          = ((unsigned char*)image->Pixels)[(image->Width * i) + j];
      else
        ((unsigned short*)thumb->Pixels)[p++]
          = ((unsigned short*)image->Pixels)[(image->Width * i) + j];

  /* copy palette */
  if (original->palette.palette)
  {
    if (!pl_image_palettize(copy,
                            original->palette.format,
                            original->palette.colors))
    {
      pl_image_destroy(copy);
      return 0;
    }

    uint pal_size = copy->palette.colors *
                    pl_image_get_bytes_per_pixel(copy->palette.format);
    memcpy(copy->palette.palette,
           original->palette.palette,
           pal_size);
  }

  /* copy image */
  memcpy(copy->bitmap,
         original->bitmap,
         get_bitmap_size(copy));

  /* copy misc. attributes */
  copy->view = original->view;

  return 1;
#endif
}

/*
void pl_image_clear(pl_image *image, uint32_t color)
{
  memset(image->bitmap, 0, image->pitch * image->height);
}
*/
/*










typedef unsigned char byte;


PspImage* pspImageRotate(const PspImage *orig, int angle_cw)
{
  PspImage *final;

  /* Create image of appropriate size *
  switch(angle_cw)
  {
  case 0:
    return pspImageCreateCopy(orig);
  case 90:
    final = pspImageCreate(orig->Viewport.Height,
      orig->Viewport.Width, orig->Depth);
    break;
  case 180:
    final = pspImageCreate(orig->Viewport.Width,
      orig->Viewport.Height, orig->Depth);
    break;
  case 270:
    final = pspImageCreate(orig->Viewport.Height,
      orig->Viewport.Width, orig->Depth);
    break;
  default:
    return NULL;
  }

  int i, j, k, di = 0;
  int width = final->Width;
  int height = final->Height;
  int l_shift = orig->BytesPerPixel >> 1;
  
  const unsigned char *source = (unsigned char*)orig->Pixels;
  unsigned char *dest = (unsigned char*)final->Pixels;

  /* Skip to Y viewport starting point *
  source += (orig->Viewport.Y * orig->Width) << l_shift;

  /* Copy image contents *
  for (i = 0, k = 0; i < orig->Viewport.Height; i++)
  {
    /* Skip to the start of the X viewport *
    source += orig->Viewport.X << l_shift;

    for (j = 0; j < orig->Viewport.Width; j++, source += 1 << l_shift, k++)
    {
      switch(angle_cw)
      {
      case 90:
        di = (width - (k / height) - 1) + (k % height) * width;
        break;
      case 180:
        di = (height - i - 1) * width + (width - j - 1);
        break;
      case 270:
        di = (k / height) + (height - (k % height) - 1) * width;
        break;
      }

      /* Write to destination *
      if (orig->Depth == PSP_IMAGE_INDEXED) dest[di] = *source;
      else ((unsigned short*)dest)[di] = *(unsigned short*)source; /* 16 bpp *
    }

    /* Skip to the end of the line *
    source += (orig->Width - (orig->Viewport.X + orig->Viewport.Width)) << l_shift;
  }

  if (orig->Depth == PSP_IMAGE_INDEXED)
  {
    memcpy(final->Palette, orig->Palette, sizeof(orig->Palette));
    final->PalSize = orig->PalSize;
  }

  return final;
}

/* Creates a half-sized thumbnail of an image *
PspImage* pspImageCreateThumbnail(const PspImage *image)
{
  PspImage *thumb;
  int i, j, p;

  if (!(thumb = pspImageCreate(image->Viewport.Width >> 1,
    image->Viewport.Height >> 1, image->Depth)))
      return NULL;

  int dy = image->Viewport.Y + image->Viewport.Height;
  int dx = image->Viewport.X + image->Viewport.Width;

  for (i = image->Viewport.Y, p = 0; i < dy; i += 2)
    for (j = image->Viewport.X; j < dx; j += 2)
      if (image->Depth == PSP_IMAGE_INDEXED)
        ((unsigned char*)thumb->Pixels)[p++]
          = ((unsigned char*)image->Pixels)[(image->Width * i) + j];
      else
        ((unsigned short*)thumb->Pixels)[p++]
          = ((unsigned short*)image->Pixels)[(image->Width * i) + j];

  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    memcpy(thumb->Palette, image->Palette, sizeof(image->Palette));
    thumb->PalSize = image->PalSize;
  }

  return thumb;
}

int pspImageDiscardColors(const PspImage *original)
{
  if (original->Depth != PSP_IMAGE_16BPP) return 0;

  int y, x, gray;
  unsigned short *p;

  for (y = 0, p = (unsigned short*)original->Pixels; y < original->Height; y++)
    for (x = 0; x < original->Width; x++, p++)
    {
      gray = (RED(*p) * 3 + GREEN(*p) * 4 + BLUE(*p) * 2) / 9;
      *p = RGB(gray, gray, gray);
    }

  return 1;
}

int pspImageBlur(const PspImage *original, PspImage *blurred)
{
  if (original->Width != blurred->Width
    || original->Height != blurred->Height
    || original->Depth != blurred->Depth
    || original->Depth != PSP_IMAGE_16BPP) return 0;

  int r, g, b, n, i, y, x, dy, dx;
  unsigned short p;

  for (y = 0, i = 0; y < original->Height; y++)
  {
    for (x = 0; x < original->Width; x++, i++)
    {
      r = g = b = n = 0;
      for (dy = y - 1; dy <= y + 1; dy++)
      {
        if (dy < 0 || dy >= original->Height) continue;

        for (dx = x - 1; dx <= x + 1; dx++)
        {
          if (dx < 0 || dx >= original->Width) continue;

          p = ((unsigned short*)original->Pixels)[dx + dy * original->Width];
          r += RED(p);
          g += GREEN(p);
          b += BLUE(p);
          n++;
        }

        r /= n;
        g /= n;
        b /= n;
        ((unsigned short*)blurred->Pixels)[i] = RGB(r, g, b);
      }
    }
  }

  return 1;
}

/* Clears an image ****
void pspImageClear(PspImage *image, unsigned int color)
{
  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    memset(image->Pixels, color & 0xff, image->Width * image->Height);
  }
  else if (image->Depth == PSP_IMAGE_16BPP)
  {
    int i;
    unsigned short *pixel = image->Pixels;
    for (i = image->Width * image->Height - 1; i >= 0; i--, pixel++)
      *pixel = color & 0xffff;
  }
}

/* Saves an image to a file ****
int pspImageSavePng(const char *path, const PspImage* image)
{
  FILE *fp = fopen( path, "wb" );
	if (!fp) return 0;

  int stat = pspImageSavePngFd(fp, image);
  fclose(fp);

  return stat;
}

#define IRGB(r,g,b,a)   (((((b)>>3)&0x1F)<<10)|((((g)>>3)&0x1F)<<5)|\
  (((r)>>3)&0x1F)|(a?0x8000:0))

/* Loads an image from an open file descriptor (16-bit PNG)****
PspImage* pspImageLoadPngFd(FILE *fp)
{
  const size_t nSigSize = 8;
  byte signature[nSigSize];
  if (fread(signature, sizeof(byte), nSigSize, fp) != nSigSize)
    return 0;

  if (!png_check_sig(signature, nSigSize))
    return 0;

  png_struct *pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!pPngStruct)
    return 0;

  png_info *pPngInfo = png_create_info_struct(pPngStruct);
  if(!pPngInfo)
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  if (setjmp(pPngStruct->jmpbuf))
  {
    png_destroy_read_struct(&pPngStruct, NULL, NULL);
    return 0;
  }

  png_init_io(pPngStruct, fp);
  png_set_sig_bytes(pPngStruct, nSigSize);
  png_read_png(pPngStruct, pPngInfo,
    PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
    PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR , NULL);

  png_uint_32 width = pPngInfo->width;
  png_uint_32 height = pPngInfo->height;
  int color_type = pPngInfo->color_type;

  PspImage *image;

  int mod_width = FindPowerOfTwoLargerThan(width);
  if (!(image = pspImageCreate(mod_width, height, PSP_IMAGE_16BPP)))
  {
    png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
    return 0;
  }

  image->Viewport.Width = width;

  png_byte **pRowTable = pPngInfo->row_pointers;
  unsigned int x, y;
  byte r, g, b, a;
  unsigned short *out = image->Pixels;

  for (y=0; y<height; y++)
  {
    png_byte *pRow = pRowTable[y];

    for (x=0; x<width; x++)
    {
      switch(color_type)
      {
        case PNG_COLOR_TYPE_GRAY:
          r = g = b = *pRow++;
          a = 1;
          break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
          r = g = b = pRow[0];
          a = pRow[1];
          pRow += 2;
          break;
        case PNG_COLOR_TYPE_RGB:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          a = 1;
          pRow += 3;
          break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
          b = pRow[0];
          g = pRow[1];
          r = pRow[2];
          a = pRow[3];
          pRow += 4;
          break;
        default:
          r = g = b = a = 0;
          break;
      }

//      *out++ = IRGB(r,g,b,a);
      *out++ = IRGB(r,g,b,a);
    }

    out += (mod_width - width);
  }

  png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);

  return image;
}

/* Saves an image to an open file descriptor (16-bit PNG)****
int pspImageSavePngFd(FILE *fp, const PspImage* image)
{
  unsigned char *bitmap;
  int i, j, width, height;

  width = image->Viewport.Width;
  height = image->Viewport.Height;

  if (!(bitmap = (u8*)malloc(sizeof(u8) * width * height * 3)))
    return 0;

  if (image->Depth == PSP_IMAGE_INDEXED)
  {
    const unsigned char *pixel;
    pixel = (unsigned char*)image->Pixels + (image->Viewport.Y * image->Width);

    for (i = 0; i < height; i++)
    {
      /* Skip to the start of the viewport ****
      pixel += image->Viewport.X;
      for (j = 0; j < width; j++, pixel++)
      {
        bitmap[i * width * 3 + j * 3 + 0] = RED(image->Palette[*pixel]);
        bitmap[i * width * 3 + j * 3 + 1] = GREEN(image->Palette[*pixel]);
        bitmap[i * width * 3 + j * 3 + 2] = BLUE(image->Palette[*pixel]);
      }
      /* Skip to the end of the line ****
      pixel += image->Width - (image->Viewport.X + width);
    }
  }
  else
  {
    const unsigned short *pixel;
    pixel = (unsigned short*)image->Pixels + (image->Viewport.Y * image->Width);

    for (i = 0; i < height; i++)
    {
      /* Skip to the start of the viewport ****
      pixel += image->Viewport.X;
      for (j = 0; j < width; j++, pixel++)
      {
        bitmap[i * width * 3 + j * 3 + 0] = RED(*pixel);
        bitmap[i * width * 3 + j * 3 + 1] = GREEN(*pixel);
        bitmap[i * width * 3 + j * 3 + 2] = BLUE(*pixel);
      }
      /* Skip to the end of the line ****
      pixel += image->Width - (image->Viewport.X + width);
    }
  }

  png_struct *pPngStruct = png_create_write_struct( PNG_LIBPNG_VER_STRING,
    NULL, NULL, NULL );

  if (!pPngStruct)
  {
    free(bitmap);
    return 0;
  }

  png_info *pPngInfo = png_create_info_struct( pPngStruct );
  if (!pPngInfo)
  {
    png_destroy_write_struct( &pPngStruct, NULL );
    free(bitmap);
    return 0;
  }

  png_byte **buf = (png_byte**)malloc(height * sizeof(png_byte*));
  if (!buf)
  {
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    free(bitmap);
    return 0;
  }

  unsigned int y;
  for (y = 0; y < height; y++)
    buf[y] = (byte*)&bitmap[y * width * 3];

  if (setjmp( pPngStruct->jmpbuf ))
  {
    free(buf);
    free(bitmap);
    png_destroy_write_struct( &pPngStruct, &pPngInfo );
    return 0;
  }

  png_init_io( pPngStruct, fp );
  png_set_IHDR( pPngStruct, pPngInfo, width, height, 8,
    PNG_COLOR_TYPE_RGB,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT);
  png_write_info( pPngStruct, pPngInfo );
  png_write_image( pPngStruct, buf );
  png_write_end( pPngStruct, pPngInfo );

  png_destroy_write_struct( &pPngStruct, &pPngInfo );
  free(buf);
  free(bitmap);

  return 1;
}

int FindPowerOfTwoLargerThan(int n)
{
  int i;
  for (i = n; i < n; i *= 2);
  return i;
}
*/

static uint get_next_power_of_two(uint n)
{
  uint i;
  for (i = 1; i < n; i <<= 1);
  return i;
}

static uint get_bitmap_size(const pl_image *image)
{
  return image->pitch * image->height;
}

int copy_from_void(pl_image_format format,
                   void *from,
                   uint32_t *to)
{
  switch (format)
  {
  case pl_image_indexed:
    *to = *(uint8_t*)from;
    return 1;
  case pl_image_5551:
  case pl_image_4444:
    *to = *(uint16_t*)from;
    return 1;
  default:
    return 0;
  }
}

int copy_to_void(pl_image_format format,
                 uint32_t from,
                 void *to)
{
  switch (format)
  {
  case pl_image_indexed:
    *(uint8_t*)to = (uint8_t)from;
    return 1;
  case pl_image_5551:
  case pl_image_4444:
    *(uint16_t*)to = (uint16_t)from;
    return 1;
  default:
    return 0;
  }
}
