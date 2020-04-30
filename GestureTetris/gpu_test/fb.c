#include "fb.h"
#include "mailbox.h"

typedef struct {
    unsigned int width;       // width of the physical screen
    unsigned int height;      // height of the physical screen
    unsigned int virtual_width;  // width of the virtual framebuffer
    unsigned int virtual_height; // height of the virtual framebuffer
    unsigned int pitch;       // number of bytes per row
    unsigned int bit_depth;   // number of bits per pixel
    unsigned int x_offset;    // x of the upper left corner of the virtual fb
    unsigned int y_offset;    // y of the upper left corner of the virtual fb
    unsigned int framebuffer; // pointer to the start of the framebuffer
    unsigned int total_bytes; // total number of bytes in the framebuffer
} fb_config_t;

typedef struct {
  unsigned int* func_addr;
  int r0;
  int r1;
  int r2;
  int r3;
  int r4;
  int r5;
} vc_func;

// fb is volatile because the GPU will write to it
static volatile fb_config_t fb __attribute__ ((aligned(16)));
static volatile vc_func gpu_test __attribute__ ((aligned(16)));

void fb_init(unsigned int width, unsigned int height, unsigned int depth_in_bytes, fb_mode_t mode)
{
    fb.width = width;
    fb.virtual_width = width;
    fb.height = height;
    fb.virtual_height = (mode == FB_SINGLEBUFFER) ? height : 2 * height;
    fb.bit_depth = depth_in_bytes * 8; // convert number of bytes to number of bits
    fb.x_offset = 0;
    fb.y_offset = 0;

    // the manual requires we to set these value to 0
    // the GPU will return new values
    fb.pitch = 0;
    fb.framebuffer = 0;
    fb.total_bytes = 0;

    // Send address of fb struct to the GPU
    mailbox_write(MAILBOX_FRAMEBUFFER, (unsigned int)&fb);
    // Read response from GPU
    mailbox_read(MAILBOX_FRAMEBUFFER);
}

int fb_gpu_func() {
  struct vc_func gpu_test;
  mailbox_write(MAILBOX_TAGS_ARM_TO_VC, (unsigned int)&gpu_test);

}

void fb_swap_buffer(void)
{
    if(fb.height != fb.virtual_height) {
      //in double buffered mode
      fb.y_offset = (fb.y_offset) ? 0 : fb.height;
      // Send address of fb struct to the GPU
      mailbox_write(MAILBOX_FRAMEBUFFER, (unsigned int)&fb);
      // Read response from GPU
      mailbox_read(MAILBOX_FRAMEBUFFER);
    }
}

void* fb_get_draw_buffer(void)
{
  if(fb.height == fb.virtual_height) {
    //single buffered mode
    return (void*) fb.framebuffer;
  }
  else {
    //double buffered mode
    if(fb.y_offset) {
      return (char*) fb.framebuffer;
    } else {
      return (char*) fb.framebuffer + fb.pitch * fb.height;
    }
  }
}

unsigned int fb_get_width(void)
{
  return fb.width;
}

unsigned int fb_get_height(void)
{
  return fb.height;
}

unsigned int fb_get_depth(void)
{
  return fb.bit_depth / 8;
}

unsigned int fb_get_pitch(void)
{
  return fb.pitch;
}
