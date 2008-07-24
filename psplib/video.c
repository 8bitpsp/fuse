/* psplib/video.c
   Graphics rendering routines

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

#include <pspgu.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <psprtc.h>

#include "video.h"

#define SLICE_SIZE 64

#define VRAM_START 0x04000000
#define VRAM_SIZE  0x00200000

const unsigned int PspFontColor[] = 
{
  0, /* Restore */
  PSP_COLOR_BLACK,
  PSP_COLOR_RED,
  PSP_COLOR_GREEN,
  PSP_COLOR_BLUE,
  PSP_COLOR_GRAY,
  PSP_COLOR_YELLOW,
  PSP_COLOR_MAGENTA,
  PSP_COLOR_WHITE
};

struct TexVertex
{
  unsigned short u, v;
  unsigned short color;
  short x, y, z;
};

static u8 FrameIndex;
static void *DisplayBuffer;
static void *DrawBuffer;
static int   PixelFormat;
static int   TexColor;
static unsigned int  VBlankFreq;
static void *VramOffset;
static void *VramChunkOffset;
static unsigned int __attribute__((aligned(16))) List[262144]; /* TODO: ? */

static inline int PutChar(const PspFont *font, int sx, int sy, unsigned char sym, int color);

void pspVideoInit()
{
  PixelFormat = GU_PSM_5551;
  TexColor = GU_COLOR_5551;
  VramOffset = 0;
  FrameIndex = 0;
  VramChunkOffset = (void*)0x44088000;

  int size;
  unsigned int vram_buffer_offset = 0;

  /* Initialize draw buffer */
  size = 2 * BUF_WIDTH * SCR_HEIGHT;
  DrawBuffer = (void*)vram_buffer_offset;
  vram_buffer_offset += size;

  /* Initialize display buffer */
  size = 4 * BUF_WIDTH * SCR_HEIGHT;
  DisplayBuffer = (void*)vram_buffer_offset;
  vram_buffer_offset += size;

  /* Initialize depth buffer */
  size = 2 * BUF_WIDTH * SCR_HEIGHT;
  void *depth_buf = (void*)vram_buffer_offset;
  vram_buffer_offset += size;

  sceGuInit();
  sceGuStart(GU_DIRECT, List);
  sceGuDrawBuffer(PixelFormat, DrawBuffer, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, DisplayBuffer, BUF_WIDTH);
  sceGuDepthBuffer(depth_buf, BUF_WIDTH);
  sceGuDisable(GU_TEXTURE_2D);
  sceGuOffset(0, 0);
  sceGuViewport(SCR_WIDTH/2, SCR_HEIGHT/2, SCR_WIDTH, SCR_HEIGHT);
  sceGuDepthRange(0xc350, 0x2710);
  sceGuDisable(GU_ALPHA_TEST);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
  sceGuEnable(GU_BLEND);
  sceGuDisable(GU_DEPTH_TEST);
  sceGuEnable(GU_CULL_FACE);
  sceGuDisable(GU_LIGHTING);
  sceGuFrontFace(GU_CW);
  sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
  sceGuAmbientColor(0xffffffff);
  sceGuFinish();
  sceGuSync(0,0);

  /* Compute VBlank frequency */
  u64 t[2];
  int i;
  for (i = 0; i < 2; i++)
  {
    sceDisplayWaitVblankStart();
    sceRtcGetCurrentTick(&t[i]);
  }
  VBlankFreq = round(1.00 / ((double)(t[1] - t[0]) 
    * (1.00 / (double)sceRtcGetTickResolution())));

  sceGuDisplay(GU_TRUE);
}

int pl_video_copy_vram(pl_image *image)
{
  sceKernelDcacheWritebackAll();

  int i, j;
  uint16_t *pixel;
  uint16_t *vram_addr = (u16*)((u8*)VRAM_START + 0x40000000);

  if (!pl_image_create(image, 
                       SCR_WIDTH,
                       SCR_HEIGHT,
                       pl_image_5551, // TODO: adapt to actual format 
                       0))
    return 0;

  for (i = 0; i < image->height; i++)
  {
    for (j = 0; j < image->view.w; j++)
    {
      pixel = (unsigned short*)image->bitmap + (i * image->line_width + j);
      *pixel = *(vram_addr + (i * BUF_WIDTH + j)) | 0x8000;
    }
  }

  return 1;
}

void pspVideoBeginList(void *list)
{
  sceGuStart(GU_CALL, list);
}

void pspVideoBegin()
{
  sceGuStart(GU_DIRECT, List);
}

void pspVideoEnd()
{
  sceGuFinish();
  sceGuSync(0, 0);
}

unsigned int GetTextureFormat(pl_image_format format)
{
  switch (format)
  {
  case pl_image_indexed:
    return GU_PSM_T8;
  case pl_image_5551:
    return GU_PSM_5551;
  case pl_image_4444:
    return GU_PSM_4444;
  default:
    return 0;
  }
}

void pl_video_put_image(const pl_image *image, 
                        int dx,
                        int dy,
                        int dw,
                        int dh)
{
  sceGuScissor(dx, dy, dx + dw, dy + dh);

  sceKernelDcacheWritebackAll();
  sceGuEnable(GU_TEXTURE_2D);

  if (image->format == pl_image_indexed)
  {
    sceGuClutMode(GetTextureFormat(image->palette.format), 0, 0xff, 0);
    sceGuClutLoad(image->palette.colors >> 3, image->palette.palette);
  }

  sceGuTexMode(GetTextureFormat(image->format), 0, 0, GU_FALSE);
  sceGuTexImage(0, image->line_width, image->line_width, image->line_width, image->bitmap);
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuTexFilter(GU_LINEAR, GU_LINEAR);

  struct TexVertex* vertices;
  int start, end, sc_end, slsz_scaled;
  slsz_scaled = ceil((float)dw * (float)SLICE_SIZE) / (float)image->view.w;

  start = image->view.x;
  end = image->view.x + image->view.w;
  sc_end = dx + dw;

  /* TODO: Convert to floating-point coords */
  for (; start < end; start += SLICE_SIZE, dx += slsz_scaled)
  {
    vertices = (struct TexVertex*)sceGuGetMemory(2 * sizeof(struct TexVertex));

    vertices[0].u = start;
    vertices[0].v = image->view.y;
    vertices[1].u = start + SLICE_SIZE;
    vertices[1].v = image->view.h + image->view.y;

    vertices[0].x = dx; vertices[0].y = dy;
    vertices[1].x = dx + slsz_scaled; vertices[1].y = dy + dh;

    vertices[0].color
      = vertices[1].color
      = vertices[0].z 
      = vertices[1].z = 0;

    sceGuDrawArray(GU_SPRITES,
      GU_TEXTURE_16BIT|TexColor|GU_VERTEX_16BIT|GU_TRANSFORM_2D,2,0,vertices);
  }

  sceGuDisable(GU_TEXTURE_2D);
  sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void pspVideoSwapBuffers()
{
  VramOffset = sceGuSwapBuffers();
  FrameIndex = !FrameIndex;
}

void pspVideoShutdown()
{
  sceGuTerm();
}

void pspVideoWaitVSync()
{
  sceDisplayWaitVblankStart();
}

void pspVideoDrawLine(int sx, int sy, int dx, int dy, u32 color)
{
  PspVertex *vert = (PspVertex*)sceGuGetMemory(sizeof(PspVertex) * 2);
  memset(vert, 0, sizeof(PspVertex) * 2);

  vert[0].x = sx; vert[0].y = sy; vert[0].color = color;
  vert[1].x = dx; vert[1].y = dy; vert[1].color = color;

  sceGuDrawArray(GU_LINES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, vert);
}

void pspVideoDrawRect(int sx, int sy, int dx, int dy, u32 color)
{
  PspVertex *vert = (PspVertex*)sceGuGetMemory(sizeof(PspVertex) * 5);
  memset(vert, 0, sizeof(PspVertex) * 5);

  vert[0].x=sx; vert[0].y=sy; vert[0].color = color;
  vert[1].x=sx; vert[1].y=dy; vert[1].color = color;
  vert[2].x=dx; vert[2].y=dy; vert[2].color = color;
  vert[3].x=dx; vert[3].y=sy; vert[3].color = color;
  vert[4].x=sx; vert[4].y=sy; vert[4].color = color;

  sceGuDrawArray(GU_LINE_STRIP, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 5, NULL, vert);
}

void pspVideoShadowRect(int sx, int sy, int dx, int dy, u32 color, int depth)
{
  int i;
  u32 alpha;
  color &= ~0xff000000;

  for (i = depth, alpha = 0x30000000; i > 0; i--, alpha += 0x20000000)
  {
    pspVideoDrawLine(sx + i, dy + i, dx + i, dy + i, color | alpha);
    pspVideoDrawLine(dx + i, sy + i, dx + i, dy + i + 1, color | alpha);
  }
}

void pspVideoGlowRect(int sx, int sy, int dx, int dy, u32 color, int radius)
{
  int i;
  u32 alpha;
  color &= ~0xff000000;

  for (i = radius, alpha = 0x30000000; i > 0; i--, alpha += 0x20000000)
    pspVideoDrawRect(sx - i, sy - i, dx + i, dy + i, color | alpha);
}

void pspVideoFillRect(int sx, int sy, int dx, int dy, u32 color)
{
  PspVertex *vert = (PspVertex*)sceGuGetMemory(4 * sizeof(PspVertex));
  memset(vert, 0, sizeof(PspVertex) * 4);

  vert[0].x = sx; vert[0].y = sy; vert[0].color = color;
  vert[1].x = dx; vert[1].y = sy; vert[1].color = color;
  vert[2].x = dx; vert[2].y = dy; vert[2].color = color;
  vert[3].x = sx; vert[3].y = dy; vert[3].color = color;

  sceGuDrawArray(GU_TRIANGLE_FAN, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 4, NULL, vert);
}

void pspVideoClearScreen()
{
  sceGuClear(GU_COLOR_BUFFER_BIT);
}

inline int PutChar(const PspFont *font, int sx, int sy, unsigned char sym, int color)
{
  /* Instead of a tab, skip 4 spaces */
  if (sym == (u8)'\t')
    return font->Chars[(int)' '].Width * 4;

  /* This function should be rewritten to write directly to VRAM, probably */
  int h, v, i, j, w, s;
  w = font->Chars[(int)sym].Width;
  h = font->Height;

  /* Allocate and clear enough memory to write the pixels of the char */
  s = sizeof(PspVertex) * (w + 2) * (h + 2);
  PspVertex *vert = (PspVertex*)sceGuGetMemory(s);
  memset(vert, 0, s);

  unsigned short row;
  int shift;

  v = 0;
  for (j = 0; j < w; j++)
    if (font->Chars[(int)sym].Char[0] & (1 << (w - j)))
    { vert[v].x = sx + j; vert[v].y = sy - 1; vert[v].color = 0xff000000; v++; }

  /* Initialize pixel values */
  for (i = 0; i < h; i++)
  {
    for (j = 0; j < w; j++)
    {
      row = font->Chars[(int)sym].Char[i];
      shift = w - j;

      if (row & (1 << shift))
      {
        if (j == 0 || !(row & (1 << (shift + 1))))
        { vert[v].x = sx + j - 1; vert[v].y = sy + i; vert[v].color = 0xff000000; v++; }
        vert[v].x = sx + j; vert[v].y = sy + i; vert[v].color = color; v++;
        vert[v].x = sx + j + 1; vert[v].y = sy + i; vert[v].color = 0xff000000; v++;
      }
      else if (i > 0 && i < h - 1)
      {
        if ((i > 0) && (font->Chars[(int)sym].Char[i - 1] & (1 << shift)))
        { vert[v].x = sx + j; vert[v].y = sy + i; vert[v].color = 0xff000000; v++; }
        else if ((i < h - 1) && (font->Chars[(int)sym].Char[i + 1] & (1 << shift)))
        { vert[v].x = sx + j; vert[v].y = sy + i; vert[v].color = 0xff000000; v++; }
      }
    }
  }

  for (j = 0; j < w; j++)
    if (font->Chars[(int)sym].Char[h - 1] & (1 << (w - j)))
    { vert[v].x = sx + j; vert[v].y = sy + h; vert[v].color = 0xff000000; v++; }

  /* Render the char as a set of pixels */
  sceGuDrawArray(GU_POINTS, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, v, NULL, vert);

  /* Return total width */
  return w;
}

int pspVideoPrint(const PspFont *font, int sx, int sy, const char *string, u32 color)
{
  return pspVideoPrintN(font, sx, sy, string, -1, color);
}

int pspVideoPrintCenter(const PspFont *font, int sx, int sy, int dx, const char *string, u32 color)
{
  const unsigned char *ch;
  int width, c = color, max;

  width = pspFontGetTextWidth(font, string);
  sx += (dx - sx) / 2 - width / 2;

  for (ch = (unsigned char*)string, width = 0, max = 0; *ch; ch++)
  {
    if (*ch < 32)
    {
      if (*ch >= PSP_FONT_RESTORE && *ch <= PSP_FONT_WHITE)
      {
        c = (*ch == PSP_FONT_RESTORE) ? color : PspFontColor[(int)(*ch) - PSP_FONT_RESTORE];
        continue;
      }
      else if (*ch == '\n')
      {
        sy += font->Height;
        width = 0;
        continue;
      }
    }

    width += PutChar(font, sx + width, sy, (u8)(*ch), c);
    if (width > max) max = width;
  }

  return max;
}

int pspVideoPrintN(const PspFont *font, int sx, int sy, const char *string, int count, u32 color)
{
  const unsigned char *ch;
  int width, i, c = color, max;

  for (ch = (unsigned char*)string, width = 0, i = 0, max = 0; *ch && (count < 0 || i < count); ch++, i++)
  {
    if (*ch < 32)
    {
      if (*ch >= PSP_FONT_RESTORE && *ch <= PSP_FONT_WHITE)
      {
        c = (*ch == PSP_FONT_RESTORE) ? color : PspFontColor[(int)(*ch) - PSP_FONT_RESTORE];
        continue;
      }
      else if (*ch == '\n')
      {
        sy += font->Height;
        width = 0;
        continue;
      }
    }

    width += PutChar(font, sx + width, sy, (u8)(*ch), c);
    if (width > max) max = width;
  }

  return max;
}

int pspVideoPrintClipped(const PspFont *font, int sx, int sy, const char* string, int max_w, char* clip, u32 color)
{
  int str_w = pspFontGetTextWidth(font, string);

  if (str_w <= max_w)
    return pspVideoPrint(font, sx, sy, string, color);

  int w, len;
  const char *ch;
  int clip_w = pspFontGetTextWidth(font, clip);

  for (ch=string, w=0, len=0; *ch && (w + clip_w < max_w); ch++, len++)
  {
    if (*ch == '\t') w += font->Chars[(u8)' '].Width * 4; 
    else w += font->Chars[(u8)(*ch)].Width;
  }

  w = pspVideoPrintN(font, sx, sy, string, len - 1, color);
  pspVideoPrint(font, sx + w, sy, clip, color);

  return w + clip_w;
}

void* pspVideoAllocateVramChunk(unsigned int bytes)
{
  void *ptr = VramChunkOffset;
  VramChunkOffset += bytes;

  return ptr;
}

unsigned int pspVideoGetVSyncFreq()
{
  return VBlankFreq;
}
