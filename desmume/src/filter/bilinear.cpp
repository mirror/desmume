/**     Code adapted from Exult source code by Forgotten
 **	Scale.cc - Trying to scale with bilinear interpolation.
 **
 **	Written: 6/14/00 - JSF
 **/

#include "filter.h"
#include "types.h"

int systemRedShift    = 16;
int systemGreenShift  = 8;
int systemBlueShift   = 0;
/*
#define RGB1(r,g,b) ((r)>>3) << systemRedShift |\
  ((g) >> 3) << systemGreenShift |\
  ((b) >> 3) << systemBlueShift\
*/
#define RGB1(r,g,b) (((r))<<systemRedShift) | (((g)) << systemGreenShift) | (((b)) << systemBlueShift)

static void fill_rgb_row_16(u16 *from, int src_width, u8 *row, int width)
{
  u8 *copy_start = row + src_width*3;
  u8 *all_stop = row + width*3;
  while (row < copy_start) {
    u16 color = *from++;
    *row++ = ((color >> systemRedShift) & 0x1f) << 3;
    *row++ = ((color >> systemGreenShift) & 0x1f) << 3;
    *row++ = ((color >> systemBlueShift) & 0x1f) << 3;
  }
  // any remaining elements to be written to 'row' are a replica of the
  // preceding pixel
  u8 *p = row-3;
  while (row < all_stop) {
    // we're guaranteed three elements per pixel; could unroll the loop
    // further, especially with a Duff's Device, but the gains would be
    // probably limited (judging by profiler output)
    *row++ = *p++;
    *row++ = *p++;
    *row++ = *p++;
  }
}

static void fill_rgb_row_32(u32 *from, int src_width, u8 *row, int width)
{
  u8 *copy_start = row + src_width*3;
  u8 *all_stop = row + width*3;
  while (row < copy_start) {
    u32 color = *from++;
    *row++ = ((color >> (systemRedShift)) ) ;
    *row++ = ((color >> (systemGreenShift)) ) ;
    *row++ = ((color >> (systemBlueShift)) ) ;
  }
  // any remaining elements to be written to 'row' are a replica of the
  // preceding pixel
  u8 *p = row-3;
  while (row < all_stop) {
    // we're guaranteed three elements per pixel; could unroll the loop
    // further, especially with a Duff's Device, but the gains would be
    // probably limited (judging by profiler output)
    *row++ = *p++;
    *row++ = *p++;
    *row++ = *p++;
  }
}

void Bilinear(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */,
              u8 *dstPtr, u32 dstPitch, int width, int height)
{
  u8 row_cur[3*322];
  u8 row_next[3*322];
  u8 *rgb_row_cur = row_cur;
  u8 *rgb_row_next = row_next;

  u16 *to = (u16 *)dstPtr;
  u16 *to_odd = (u16 *)(dstPtr + dstPitch);

  int from_width = width;
  u16 *from = (u16 *)srcPtr;
  fill_rgb_row_16(from, from_width, rgb_row_cur, width+1);

  for(int y = 0; y < height; y++) {
    u16 *from_orig = from;
    u16 *to_orig = to;

    if (y+1 < height)
      fill_rgb_row_16(from+width, from_width, rgb_row_next,
                   width+1);
    else
      fill_rgb_row_16(from, from_width, rgb_row_next, width+1);

    // every pixel in the src region, is extended to 4 pixels in the
    // destination, arranged in a square 'quad'; if the current src
    // pixel is 'a', then in what follows 'b' is the src pixel to the
    // right, 'c' is the src pixel below, and 'd' is the src pixel to
    // the right and down
    u8 *cur_row  = rgb_row_cur;
    u8 *next_row = rgb_row_next;
    u8 *ar = cur_row++;
    u8 *ag = cur_row++;
    u8 *ab = cur_row++;
    u8 *cr = next_row++;
    u8 *cg = next_row++;
    u8 *cb = next_row++;
    for(int x=0; x < width; x++) {
      u8 *br = cur_row++;
      u8 *bg = cur_row++;
      u8 *bb = cur_row++;
      u8 *dr = next_row++;
      u8 *dg = next_row++;
      u8 *db = next_row++;

      // upper left pixel in quad: just copy it in
      *to++ = RGB1(*ar, *ag, *ab);

      // upper right
      *to++ = RGB1((*ar+*br)>>1, (*ag+*bg)>>1, (*ab+*bb)>>1);

      // lower left
      *to_odd++ = RGB1((*ar+*cr)>>1, (*ag+*cg)>>1, (*ab+*cb)>>1);

      // lower right
      *to_odd++ = RGB1((*ar+*br+*cr+*dr)>>2,
                      (*ag+*bg+*cg+*dg)>>2,
                      (*ab+*bb+*cb+*db)>>2);

      // 'b' becomes 'a', 'd' becomes 'c'
      ar = br;
      ag = bg;
      ab = bb;
      cr = dr;
      cg = dg;
      cb = db;
    }

    // the "next" rgb row becomes the current; the old current rgb row is
    // recycled and serves as the new "next" row
    u8 *temp;
    temp = rgb_row_cur;
    rgb_row_cur = rgb_row_next;
    rgb_row_next = temp;

    // update the pointers for start of next pair of lines
    from = (u16 *)((u8 *)from_orig + srcPitch);
    to = (u16 *)((u8 *)to_orig + (dstPitch << 1));
    to_odd = (u16 *)((u8 *)to + dstPitch);
  }
}

void BilinearPlus(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */,
                  u8 *dstPtr, u32 dstPitch, int width, int height)
{
  u8 row_cur[3*322];
  u8 row_next[3*322];
  u8 *rgb_row_cur = row_cur;
  u8 *rgb_row_next = row_next;

  u16 *to = (u16 *)dstPtr;
  u16 *to_odd = (u16 *)(dstPtr + dstPitch);

  int from_width = width;
  u16 *from = (u16 *)srcPtr;
  fill_rgb_row_16(from, from_width, rgb_row_cur, width+1);

  for(int y = 0; y < height; y++) {
    u16 *from_orig = from;
    u16 *to_orig = to;

    if (y+1 < height)
      fill_rgb_row_16(from+width, from_width, rgb_row_next,
                   width+1);
    else
      fill_rgb_row_16(from, from_width, rgb_row_next, width+1);

    // every pixel in the src region, is extended to 4 pixels in the
    // destination, arranged in a square 'quad'; if the current src
    // pixel is 'a', then in what follows 'b' is the src pixel to the
    // right, 'c' is the src pixel below, and 'd' is the src pixel to
    // the right and down
    u8 *cur_row  = rgb_row_cur;
    u8 *next_row = rgb_row_next;
    u8 *ar = cur_row++;
    u8 *ag = cur_row++;
    u8 *ab = cur_row++;
    u8 *cr = next_row++;
    u8 *cg = next_row++;
    u8 *cb = next_row++;
    for(int x=0; x < width; x++) {
      u8 *br = cur_row++;
      u8 *bg = cur_row++;
      u8 *bb = cur_row++;
      u8 *dr = next_row++;
      u8 *dg = next_row++;
      u8 *db = next_row++;

      // upper left pixel in quad: just copy it in
      //*to++ = manip.rgb(*ar, *ag, *ab);
#ifdef USE_ORIGINAL_BILINEAR_PLUS
      *to++ = RGB(
                  (((*ar)<<2) +((*ar)) + (*cr+*br+*br) )>> 3,
                  (((*ag)<<2) +((*ag)) + (*cg+*bg+*bg) )>> 3,
                  (((*ab)<<2) +((*ab)) + (*cb+*bb+*bb) )>> 3);
#else
      *to++ = RGB1(
                  (((*ar)<<3) +((*ar)<<1) + (*cr+*br+*br+*cr) )>> 4,
                  (((*ag)<<3) +((*ag)<<1) + (*cg+*bg+*bg+*cg) )>> 4,
                  (((*ab)<<3) +((*ab)<<1) + (*cb+*bb+*bb+*cb) )>> 4);
#endif

      // upper right
      *to++ = RGB1((*ar+*br)>>1, (*ag+*bg)>>1, (*ab+*bb)>>1);

      // lower left
      *to_odd++ = RGB1((*ar+*cr)>>1, (*ag+*cg)>>1, (*ab+*cb)>>1);

      // lower right
      *to_odd++ = RGB1((*ar+*br+*cr+*dr)>>2,
                      (*ag+*bg+*cg+*dg)>>2,
                      (*ab+*bb+*cb+*db)>>2);

      // 'b' becomes 'a', 'd' becomes 'c'
      ar = br;
      ag = bg;
      ab = bb;
      cr = dr;
      cg = dg;
      cb = db;
    }

    // the "next" rgb row becomes the current; the old current rgb row is
    // recycled and serves as the new "next" row
    u8 *temp;
    temp = rgb_row_cur;
    rgb_row_cur = rgb_row_next;
    rgb_row_next = temp;

    // update the pointers for start of next pair of lines
    from = (u16 *)((u8 *)from_orig + srcPitch);
    to = (u16 *)((u8 *)to_orig + (dstPitch << 1));
    to_odd = (u16 *)((u8 *)to + dstPitch);
  }
}

void Bilinear32(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */,
                u8 *dstPtr, u32 dstPitch, int width, int height)
{
  u8 row_cur[3*322];
  u8 row_next[3*322];
  u8 *rgb_row_cur = row_cur;
  u8 *rgb_row_next = row_next;

  u32 *to = (u32 *)dstPtr;
  u32 *to_odd = (u32 *)(dstPtr + dstPitch);

  int from_width = width;

  u32 *from = (u32 *)srcPtr;
  fill_rgb_row_32(from, from_width, rgb_row_cur, width+1);

  for(int y = 0; y < height; y++) {
    u32 *from_orig = from;
    u32 *to_orig = to;

    if (y+1 < height)
      fill_rgb_row_32(from+width+1, from_width, rgb_row_next,
                   width+1);
    else
      fill_rgb_row_32(from, from_width, rgb_row_next, width+1);

    // every pixel in the src region, is extended to 4 pixels in the
    // destination, arranged in a square 'quad'; if the current src
    // pixel is 'a', then in what follows 'b' is the src pixel to the
    // right, 'c' is the src pixel below, and 'd' is the src pixel to
    // the right and down
    u8 *cur_row  = rgb_row_cur;
    u8 *next_row = rgb_row_next;
    u8 *ar = cur_row++;
    u8 *ag = cur_row++;
    u8 *ab = cur_row++;
    u8 *cr = next_row++;
    u8 *cg = next_row++;
    u8 *cb = next_row++;
    for(int x=0; x < width; x++) {
      u8 *br = cur_row++;
      u8 *bg = cur_row++;
      u8 *bb = cur_row++;
      u8 *dr = next_row++;
      u8 *dg = next_row++;
      u8 *db = next_row++;

      // upper left pixel in quad: just copy it in
	  int m = *ar;
	  int mm = *ag;
	  int mmmm = *ab;
	  int mmm =  RGB1(*ar, *ag, *ab);
      *to++ = RGB1(*ar, *ag, *ab);

      // upper right
      *to++ = RGB1((*ar+*br)>>1, (*ag+*bg)>>1, (*ab+*bb)>>1);

      // lower left
      *to_odd++ = RGB1((*ar+*cr)>>1, (*ag+*cg)>>1, (*ab+*cb)>>1);

      // lower right
      *to_odd++ = RGB1((*ar+*br+*cr+*dr)>>2,
                      (*ag+*bg+*cg+*dg)>>2,
                      (*ab+*bb+*cb+*db)>>2);

      // 'b' becomes 'a', 'd' becomes 'c'
      ar = br;
      ag = bg;
      ab = bb;
      cr = dr;
      cg = dg;
      cb = db;
    }

    // the "next" rgb row becomes the current; the old current rgb row is
    // recycled and serves as the new "next" row
    u8 *temp;
    temp = rgb_row_cur;
    rgb_row_cur = rgb_row_next;
    rgb_row_next = temp;

    // update the pointers for start of next pair of lines
    from = (u32 *)((u8 *)from_orig + srcPitch);
    to = (u32 *)((u8 *)to_orig + (dstPitch << 1));
    to_odd = (u32 *)((u8 *)to + dstPitch);
  }
}

void BilinearPlus32(u8 *srcPtr, u32 srcPitch, u8 * /* deltaPtr */,
                    u8 *dstPtr, u32 dstPitch, int width, int height)
{
  u8 row_cur[3*322];
  u8 row_next[3*322];
  u8 *rgb_row_cur = row_cur;
  u8 *rgb_row_next = row_next;

  u32 *to = (u32 *)dstPtr;
  u32 *to_odd = (u32 *)(dstPtr + dstPitch);

  int from_width = width;

  u32 *from = (u32 *)srcPtr;
  fill_rgb_row_32(from, from_width, rgb_row_cur, width+1);

  for(int y = 0; y < height; y++) {
    u32 *from_orig = from;
    u32 *to_orig = to;

    if (y+1 < height)
      fill_rgb_row_32(from+width+1, from_width, rgb_row_next,
                   width+1);
    else
      fill_rgb_row_32(from, from_width, rgb_row_next, width+1);

    // every pixel in the src region, is extended to 4 pixels in the
    // destination, arranged in a square 'quad'; if the current src
    // pixel is 'a', then in what follows 'b' is the src pixel to the
    // right, 'c' is the src pixel below, and 'd' is the src pixel to
    // the right and down
    u8 *cur_row  = rgb_row_cur;
    u8 *next_row = rgb_row_next;
    u8 *ar = cur_row++;
    u8 *ag = cur_row++;
    u8 *ab = cur_row++;
    u8 *cr = next_row++;
    u8 *cg = next_row++;
    u8 *cb = next_row++;
    for(int x=0; x < width; x++) {
      u8 *br = cur_row++;
      u8 *bg = cur_row++;
      u8 *bb = cur_row++;
      u8 *dr = next_row++;
      u8 *dg = next_row++;
      u8 *db = next_row++;

      // upper left pixel in quad: just copy it in
      //*to++ = manip.rgb(*ar, *ag, *ab);
#ifdef USE_ORIGINAL_BILINEAR_PLUS
      *to++ = RGB(
                  (((*ar)<<2) +((*ar)) + (*cr+*br+*br) )>> 3,
                  (((*ag)<<2) +((*ag)) + (*cg+*bg+*bg) )>> 3,
                  (((*ab)<<2) +((*ab)) + (*cb+*bb+*bb) )>> 3);
#else
      *to++ = RGB1(
                  (((*ar)<<3) +((*ar)<<1) + (*cr+*br+*br+*cr) )>> 4,
                  (((*ag)<<3) +((*ag)<<1) + (*cg+*bg+*bg+*cg) )>> 4,
                  (((*ab)<<3) +((*ab)<<1) + (*cb+*bb+*bb+*cb) )>> 4);
#endif

      // upper right
      *to++ = RGB1((*ar+*br)>>1, (*ag+*bg)>>1, (*ab+*bb)>>1);

      // lower left
      *to_odd++ = RGB1((*ar+*cr)>>1, (*ag+*cg)>>1, (*ab+*cb)>>1);

      // lower right
      *to_odd++ = RGB1((*ar+*br+*cr+*dr)>>2,
                      (*ag+*bg+*cg+*dg)>>2,
                      (*ab+*bb+*cb+*db)>>2);

      // 'b' becomes 'a', 'd' becomes 'c'
      ar = br;
      ag = bg;
      ab = bb;
      cr = dr;
      cg = dg;
      cb = db;
    }

    // the "next" rgb row becomes the current; the old current rgb row is
    // recycled and serves as the new "next" row
    u8 *temp;
    temp = rgb_row_cur;
    rgb_row_cur = rgb_row_next;
    rgb_row_next = temp;

    // update the pointers for start of next pair of lines
    from = (u32 *)((u8 *)from_orig + srcPitch);
    to = (u32 *)((u8 *)to_orig + (dstPitch << 1));
    to_odd = (u32 *)((u8 *)to + dstPitch);
  }
}
void RenderBilinear (SSurface Src, SSurface Dst)
{

    unsigned char *lpSrc, *lpDst;

    lpSrc = Src.Surface;
    lpDst = Dst.Surface;

    Bilinear32 (lpSrc, Src.Pitch*2,
                lpSrc,
                lpDst, Dst.Pitch*2, Src.Width, Src.Height);
}

