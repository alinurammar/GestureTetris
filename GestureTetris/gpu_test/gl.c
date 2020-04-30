#include "gl.h"
#include "fb.h"
#include "font.h"

int min(int a, int b);

static void* framebuffer;

void gl_init(unsigned int width, unsigned int height, gl_mode_t mode)
{
    fb_init(width, height, 4, mode);    // use 32-bit depth always for graphics library
    framebuffer = fb_get_draw_buffer();
}

static inline const int in_bounds(int x, int y) {
  return x >= 0 && x < gl_get_width() && y >= 0 && y < gl_get_height();
}

void gl_swap_buffer(void)
{
    fb_swap_buffer();
    framebuffer = fb_get_draw_buffer();
}

unsigned int gl_get_width(void)
{
    return fb_get_width();
}

unsigned int gl_get_height(void)
{
    return fb_get_height();
}

color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    return (0xff << 24) + (r << 16) + (g << 8) + b;
}

void gl_clear(color_t c)
{
    gl_draw_rect(0, 0, gl_get_width(), gl_get_height(), c);
}

void gl_draw_pixel(int x, int y, color_t c)
{
    if(in_bounds(x, y)) {
       int width = fb_get_pitch() / 4;
       unsigned int (*pixels)[width] = (unsigned int (*)[width]) framebuffer;
       pixels[y][x] = c;
    }
}

color_t gl_read_pixel(int x, int y)
{
    if(in_bounds(x, y)) {
       int width = fb_get_pitch() / 4;
       unsigned int (*pixels)[width] = (unsigned int (*)[width]) framebuffer;
       return pixels[y][x];
    }
    return 0;
}

void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    if(in_bounds(x, y)) {
      int width = fb_get_pitch() / 4;
      unsigned int (*pixels)[width] = (unsigned int (*)[width]) framebuffer;
      int xBound = min(x + w, gl_get_width());
      int yBound = min(y + h, gl_get_height());
      for(int xPos = x; xPos < xBound; xPos++) {
        for(int yPos = y; yPos < yBound; yPos++) {
          pixels[yPos][xPos] = c;
        }
      }
    }
}

void gl_draw_char(int x, int y, int ch, color_t c)
{
    unsigned char charData[font_get_size()];
    if(font_get_char(ch, charData, font_get_size())) {
      int width = font_get_width();
      int height = font_get_height();
      char (*charPix)[width] = (char (*)[width]) charData;
      //draw character
      for(int delX = 0; delX < width; delX++) {
        for(int delY = 0; delY < height; delY++) {
          if(charPix[delY][delX]) {
            gl_draw_pixel(x + delX, y + delY, c);
          }
        }
      }
    }
}

void gl_draw_string(int x, int y, const char* str, color_t c)
{
    int width = font_get_width();
    for(const char* i = str; *i != '\0'; i++) {
      gl_draw_char(x + (i - str) * width, y, *i, c);
    }
}

unsigned int gl_get_char_height(void)
{
    return font_get_height();
}

unsigned int gl_get_char_width(void)
{
    return font_get_width();
}
