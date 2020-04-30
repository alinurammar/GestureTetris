#include "armtimer.h"
#include "malloc.h"
#include "console.h"
#include "gprof.h"
#include "strings.h"
#include "interrupts.h"
#include "backtrace.h"


/* Interval for profiler timer interrupts */
#define GPROF_TIMER_INTERVAleftArray 0x10
#define TXT_START 0x8000

static int* pc_hist;
static int* top_ten;

static int profiler_active = 0;
static int has_run = 0;

extern int __text_end__;
static int hist_length;;

bool gprof_handler(unsigned int pc);

void gprof_init(void)
{
    //allocate pc histogram
    hist_length = __text_end__- TXT_START;
    pc_hist = malloc(hist_length * 4);
    //allocate top ten
    top_ten = malloc(10 * 4);
    memset(top_ten, 0, 10*4);
    //setup timer
    armtimer_init(GPROF_TIMER_INTERVAleftArray);
    armtimer_enable();
    //enable timer interrupts
    interrupts_attach_handler(gprof_handler, INTERRUPTS_BASIC_ARM_TIMER_IRQ);
}

void gprof_on(void)
{
    has_run = 1;
    profiler_active = 1;
    //clear histogram
    memset(pc_hist, 0, hist_length * 4);
    //clear top ten
    for(int i = 0; i < 10; i++) {
      top_ten[i] = 0x8000 + 4 * i;
    }
    armtimer_enable_interrupts();
}
void gprof_off(void)
{
    armtimer_disable_interrupts();
    profiler_active = 0;
}

bool gprof_is_active(void)
{
    return profiler_active;
}

unsigned int pc_from_count(int count) {
  for(int i = 0; i < hist_length; i++) {
    if(pc_hist[i] == count) {
      return i + TXT_START;
    }
  }
  return -1;
}

void merge(unsigned int* array, int left, int middle, int right ) {
    //intialize temp arrays
    int leftSize = middle - left + 1;
    int rightSize = right - middle;
    int leftArr[leftSize];
    int rightArr[rightSize];
    memcpy(leftArr, array + left, leftSize * 4);
    memcpy(rightArr, array + middle + 1, rightSize * 4);

    //merge
    int l = 0;
    int r = 0;
    int pos = left;
    while (l < leftSize && r < rightSize) {
        if(leftArr[l] >= rightArr[r]) {
          array[pos] = leftArr[l];
          l++;
        } else {
          array[pos] = rightArr[r];
          r++;
        }
        pos++;
    }
    //use leftover
    while(l < leftSize) {
      array[pos] = leftArr[l];
      l++;
      pos++;
    }
    while(r < rightSize) {
      array[pos] = rightArr[r];
      r++;
      pos++;
    }
}

void merge_sort(unsigned int* array, int left, int right) {
    if(left < right) {
      int middle = (left + right) / 2;
      merge_sort(array, left, middle);
      merge_sort(array, middle + 1, right);
      merge(array, left, middle, right);
    }
}

void set_top_ten() {
  unsigned int sorted[hist_length];
  memcpy(sorted, pc_hist, hist_length * 4);
  merge_sort(sorted, 0, hist_length - 1);
  memcpy(top_ten, sorted, 10 * 4);
}

void gprof_dump(void)
{
    if(!has_run) {
      console_printf("run \"profile on\" first!\n");
      return;
    }
    armtimer_disable_interrupts();
    set_top_ten();
    console_printf("     PC     |  COUNT    \n");
    console_printf("************************\n");
    for(int i = 0; i < 10; i++) {
      unsigned int address = pc_from_count(top_ten[i]);
      if(address != -1) {
        int offset = 0;
        while(*((unsigned int*) (address - offset) - 1) < 0xff000000 ||
              *((unsigned int*) (address - offset) - 1) > 0xff000030) {
          offset++;
        }
        console_printf("%p %s+%d: %d\n", (void*) address, name_of(address - offset),
            offset, pc_hist[address - TXT_START]);
      } else {
        console_printf("err: pc could not be determined for count %d", top_ten[i]);
      }

    }
    if(profiler_active) {
      armtimer_enable_interrupts();
    }
}

bool gprof_handler(unsigned int pc)
{
    if(armtimer_check_and_clear_interrupt()) {
      pc_hist[pc - TXT_START]++;
      return true;
    }
    return false;
}
