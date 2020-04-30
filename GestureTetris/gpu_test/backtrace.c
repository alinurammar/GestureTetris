#include "backtrace.h"
#include "printf.h"

uintptr_t* next_fp(uintptr_t*);

int backtrace (frame_t f[], int max_frames)
{
    //get current fp using inline assembly
    uintptr_t* cur_fp;
    __asm__("mov %0, fp" : "=r" (cur_fp));
    //backtrace
    int done_frames = 0;
    while(done_frames < max_frames) {
        //set lr
        f[done_frames].resume_addr = *(cur_fp - 1);
        //advance to next stack frame
        cur_fp = next_fp(cur_fp);
        if(!cur_fp) {
          break;
        }
        //set name
        f[done_frames].name = name_of(*cur_fp - 12);
        //set resume_offset
        f[done_frames].resume_offset = f[done_frames].resume_addr - (*cur_fp - 12);
         done_frames++;
    }
    return done_frames;
}

uintptr_t* next_fp(uintptr_t* cur_fp) {
  return  (uintptr_t*) *(cur_fp - 3);
}

void print_frames (frame_t f[], int n)
{
    for (int i = 0; i < n; i++)
        printf("#%d 0x%x at %s+%d\n", i, f[i].resume_addr, f[i].name, f[i].resume_offset);
}

void print_backtrace (void)
{
    int max = 50;
    frame_t arr[max];

    int n = backtrace(arr, max);
    print_frames(arr+1, n-1);   // print frames starting at this function's caller
}

const char *name_of(uintptr_t fn_start_addr) {
    uint32_t value = *((unsigned int*) fn_start_addr - 1);
    if((value >> 24) == 0xff) {
      return (const char*) (fn_start_addr - (value & ~0xff000000) - 4);
    }
    return "???";
}
