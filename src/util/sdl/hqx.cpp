#ifdef USE_SDL

/* by byuu
 * originally from http://nesdev.parodius.com/bbs/viewtopic.php?p=82770#82770
 * http://pastebin.com/YXpmqvW5
 */

#include <stdint.h>
#include "util/bitmap.h"
#include <SDL/SDL.h>

namespace hq2x{

enum {
    /* 440: 10001000000
     * 207: 01000000111
     * 407: 10000000111
     */
    diff_offset = (0x440 << 21) + (0x207 << 11) + 0x407,
    /* 380: 0b1110000000
     * 1f0: 0b0111110000
     * 3f0: 0b1111110000
     */
    diff_mask   = (0x380 << 21) + (0x1f0 << 11) + 0x3f0,
};

uint32_t *yuvTable;
uint16_t *rgb565Table;
uint8_t rotate[256];

/* FIXME: replace these with readable constant names */
const uint8_t hqTable[256] = {
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 15, 12, 5,  3, 17, 13,
  4, 4, 6, 18, 4, 4, 6, 18, 5,  3, 12, 12, 5,  3,  1, 12,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 17, 13, 5,  3, 16, 14,
  4, 4, 6, 18, 4, 4, 6, 18, 5,  3, 16, 12, 5,  3,  1, 14,
  4, 4, 6,  2, 4, 4, 6,  2, 5, 19, 12, 12, 5, 19, 16, 12,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3, 16, 12,
  4, 4, 6,  2, 4, 4, 6,  2, 5, 19,  1, 12, 5, 19,  1, 14,
  4, 4, 6,  2, 4, 4, 6, 18, 5,  3, 16, 12, 5, 19,  1, 14,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 15, 12, 5,  3, 17, 13,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3, 16, 12,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 17, 13, 5,  3, 16, 14,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 13, 5,  3,  1, 14,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3, 16, 13,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3,  1, 12,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3, 16, 12, 5,  3,  1, 14,
  4, 4, 6,  2, 4, 4, 6,  2, 5,  3,  1, 12, 5,  3,  1, 14,
};

static void rgb555_to_rgb888(uint8_t red, uint8_t green, uint8_t blue,
                             uint8_t & red_output, uint8_t & green_output, uint8_t & blue_output){
    red_output = (red << 3) | (red >> 2);
    green_output = (green << 3) | (green >> 2);
    blue_output = (blue << 3) | (blue >> 2);
}

static void rgb565_to_rgb888(uint8_t red, uint8_t green, uint8_t blue,
                             uint8_t & red_output, uint8_t & green_output, uint8_t & blue_output){
    red_output = (red << 3) | (red >> 2);
    green_output = (green << 2) | (green >> 4);
    blue_output = (blue << 3) | (blue >> 2);
}

static void rgb888_to_rgb565(uint8_t red, uint8_t green, uint8_t blue,
                             uint8_t & red_output, uint8_t & green_output, uint8_t & blue_output){
    red_output = red >> 3;
    green_output = green >> 2;
    blue_output = blue >> 3;
}

static void rgb888_to_yuv888(uint8_t red, uint8_t green, uint8_t blue,
                             double & y, double & u, double & v){
    y = (red + green + blue) * (0.25f * (63.5f / 48.0f));
    u = ((red - blue) * 0.25f + 128.0f) * (7.5f / 7.0f);
    v = ((green * 2.0f - red - blue) * 0.125f + 128.0f) * (7.5f / 6.0f);
}

static void yuv888_to_rgb888(double y, double u, double v,
                             uint8_t & red, uint8_t & green, uint8_t & blue){
    blue = 1.164 * (y - 16) + 2.018 * (u - 128);
    green = 1.164 * (y - 16) - 0.813 * (v - 128) - 0.391 * (u - 128);
    red = 1.164 * (y - 16) + 1.596 * (v - 128);
}

static void initialize() {
    static bool initialized = false;
    if (initialized == true){
        return;
    }
    initialized = true;

    rgb565Table = new uint16_t[65536];
    yuvTable = new uint32_t[65536];

    for (unsigned int i = 0; i < 65536; i++) {
        uint8_t B = (i >>  0) & 31;
        uint8_t G = (i >>  5) & 63;
        uint8_t R = (i >> 11) & 31;

        uint8_t r, g, b;
        rgb565_to_rgb888(R, G, B, r, g, b);

        double y, u, v;
        rgb888_to_yuv888(r, g, b, y, u, v);

        rgb565Table[i] = Graphics::makeColor(r, g, b);

        /* Pack the YUV parameters into a single int */
        yuvTable[i] = ((unsigned)y << 21) + ((unsigned)u << 11) + ((unsigned)v);
    }

    for (unsigned int n = 0; n < 256; n++){
        /* What does this do? */
        rotate[n] = ((n >> 2) & 0x11) | ((n << 2) & 0x88) |
                    ((n & 0x01) << 5) | ((n & 0x08) << 3) |
                    ((n & 0x10) >> 3) | ((n & 0x80) >> 5);
    }
}

static void terminate() {
  delete[] yuvTable;
  delete[] rgb565Table;
}

static bool same(uint16_t x, uint16_t y) {
  return !((yuvTable[x] - yuvTable[y] + diff_offset) & diff_mask);
}

static bool diff(uint32_t x, uint16_t y) {
  return ((x - yuvTable[y]) & diff_mask);
}

/* 0x03e07c1f: 11111000000111110000011111 */
/* I'm pretty sure grow/pack are used to do psuedo-simd operations on pixels,
 * operations on all the components can be done in parallel because they are separated
 * by enough bits.
 * Grow duplicates values, pack unduplicates them.
 *
 */

/* 0x03e07e1f: 011111000000111111000011111 */
/* 0x03e07c1f: 011111000000111110000011111 */
/* 0x03e0fc1f: 011111000001111110000011111 */
/* 0x07E0F81F: 111111000001111100000011111 */

#define RGB32_555 0x3e07c1f
// #define RGB32_565 0x3e0fc1f 
#define RGB32_565 0x7E0F81F
static void grow(uint32_t &n) { n |= n << 16; n &= RGB32_565; }
static uint16_t pack(uint32_t n) { n &= RGB32_565; return n | (n >> 16); }

static uint16_t blend1(uint32_t A, uint32_t B) {
  grow(A); grow(B);
  A = (A * 3 + B) >> 2;
  return pack(A);
}

static uint16_t blend2(uint32_t A, uint32_t B, uint32_t C) {
  grow(A); grow(B); grow(C);
  return pack((A * 2 + B + C) >> 2);
}

static uint16_t blend3(uint32_t A, uint32_t B, uint32_t C) {
  grow(A); grow(B); grow(C);
  return pack((A * 5 + B * 2 + C) >> 3);
}

static uint16_t blend4(uint32_t A, uint32_t B, uint32_t C) {
  grow(A); grow(B); grow(C);
  return pack((A * 6 + B + C) >> 3);
}

static uint16_t blend5(uint32_t A, uint32_t B, uint32_t C) {
  grow(A); grow(B); grow(C);
  return pack((A * 2 + (B + C) * 3) >> 3);
}

static uint16_t blend6(uint32_t A, uint32_t B, uint32_t C) {
  grow(A); grow(B); grow(C);
  return pack((A * 14 + B + C) >> 4);
}

static uint16_t blend(unsigned rule, uint16_t E, uint16_t A, uint16_t B, uint16_t D, uint16_t F, uint16_t H) {
    switch(rule) { default:
        case  0: return E;
        case  1: return blend1(E, A);
        case  2: return blend1(E, D);
        case  3: return blend1(E, B);
        case  4: return blend2(E, D, B);
        case  5: return blend2(E, A, B);
        case  6: return blend2(E, A, D);
        case  7: return blend3(E, B, D);
        case  8: return blend3(E, D, B);
        case  9: return blend4(E, D, B);
        case 10: return blend5(E, D, B);
        case 11: return blend6(E, D, B);
        case 12: return same(B, D) ? blend2(E, D, B) : E;
        case 13: return same(B, D) ? blend5(E, D, B) : E;
        case 14: return same(B, D) ? blend6(E, D, B) : E;
        case 15: return same(B, D) ? blend2(E, D, B) : blend1(E, A);
        case 16: return same(B, D) ? blend4(E, D, B) : blend1(E, A);
        case 17: return same(B, D) ? blend5(E, D, B) : blend1(E, A);
        case 18: return same(B, F) ? blend3(E, B, D) : blend1(E, D);
        case 19: return same(D, H) ? blend3(E, D, B) : blend1(E, B);
    }
}

/* why is this here */
void filter_size(unsigned &width, unsigned &height) {
    initialize();
    width  *= 2;
    height *= 2;
}

void filter_render(uint16_t *colortable, SDL_Surface * input, SDL_Surface * output){
    initialize();
    /* pitch is adjusted by the bit depth (2 bytes per pixel) so we
     * divide by two since we are using int16_t
     */
    int pitch = input->pitch / 2;
    int outpitch = output->pitch / 2;

    //#pragma omp parallel for
    for (unsigned int y = 0; y < input->h; y++){
        const uint16_t * in = (uint16_t*) input->pixels + y * pitch;
        uint16_t *out0 = (uint16_t*) output->pixels + y * outpitch * 2;
        uint16_t *out1 = (uint16_t*) output->pixels + y * outpitch * 2 + outpitch;

        int prevline = (y == 0 ? 0 : pitch);
        int nextline = (y == input->h - 1 ? 0 : pitch);

        // *out0 = Graphics::makeColor(255, 255, 255);
        *out0++ = *in; *out0++ = *in;
        *out1++ = *in; *out1++ = *in;
        in++;

        for (unsigned int x = 1; x < input->w - 1; x++){
            uint16_t A = *(in - prevline - 1);
            uint16_t B = *(in - prevline + 0);
            uint16_t C = *(in - prevline + 1);
            uint16_t D = *(in - 1);
            uint16_t E = *(in + 0);
            uint16_t F = *(in + 1);
            uint16_t G = *(in + nextline - 1);
            uint16_t H = *(in + nextline + 0);
            uint16_t I = *(in + nextline + 1);
            uint32_t e = yuvTable[E] + diff_offset;

            uint8_t pattern;
            pattern  = diff(e, A) << 0;
            pattern |= diff(e, B) << 1;
            pattern |= diff(e, C) << 2;
            pattern |= diff(e, D) << 3;
            pattern |= diff(e, F) << 4;
            pattern |= diff(e, G) << 5;
            pattern |= diff(e, H) << 6;
            pattern |= diff(e, I) << 7;

            /* upper left */
            *(out0 + 0) = colortable[blend(hqTable[pattern], E, A, B, D, F, H)]; pattern = rotate[pattern];
            /* upper right */
            *(out0 + 1) = colortable[blend(hqTable[pattern], E, C, F, B, H, D)]; pattern = rotate[pattern];
            /* lower right */
            *(out1 + 1) = colortable[blend(hqTable[pattern], E, I, H, F, D, B)]; pattern = rotate[pattern];
            /* lower left */
            *(out1 + 0) = colortable[blend(hqTable[pattern], E, G, D, H, B, F)];

            in++;
            out0 += 2;
            out1 += 2;
        }

        *out0++ = *in; *out0++ = *in;
        *out1++ = *in; *out1++ = *in;
        in++;
    }
}

void filter_render_565(SDL_Surface * input, SDL_Surface * output){
    initialize();
    filter_render(rgb565Table, input, output);
}

}

#endif
