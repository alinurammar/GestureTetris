#include "malloc.h"
#include "malloc_internal.h"
#include "printf.h"
#include <stddef.h> // for NULL
#include "strings.h"
#include "backtrace.h"

extern int __bss_end__;

#define STACK_START 0x8000000
#define STACK_SIZE  0x1000000
#define STACK_END ((char *)STACK_START - STACK_SIZE)

//48 bytes
struct header {
    size_t payload_size;
    frame_t trace[3];
    int status;
    char redzone[4];
};

int malloc_calls = 0;
int free_calls = 0;
int bytes_allocated = 0;

struct header* next_block(struct header* block);
char* far_redzone(struct header* block);

/*
 * The pool of memory available for the heap starts at the upper end of the
 * data section and can extend from there up to the lower end of the stack.
 * It uses symbol __bss_end__ from memmap to locate data end
 * and calculates stack end assuming a 16MB stack.
 *
 * Global variables for the bump allocator:
 *
 * `heap_start`  location where heap segment starts
 * `heap_end`    location at end of in-use portion of heap segment
 */

// Initial heap segment starts at bss_end and is empty
static void *heap_start = &__bss_end__;
static void *heap_end = &__bss_end__;

void *sbrk(int nbytes)
{
    void *prev_end = heap_end;
    if ((char *)prev_end + nbytes > STACK_END) {
        return NULL;
    } else {
        heap_end = (char *)prev_end + nbytes;
        return prev_end;
    }
}

// Simple macro to round up x to multiple of n.
// The efficient but tricky bitwise approach it uses
// works only if n is a power of two -- why?
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

void *malloc (size_t nbytes)
{
    malloc_calls++;
    if(nbytes == 0) {
      return NULL;
    }
    nbytes = roundup(nbytes + 4, 8);
    bytes_allocated+=nbytes;
    //search for freespace
    struct header* loc = heap_start;
    while((char*) loc < (char*) heap_end) {
      if(!loc->status && (loc->payload_size > nbytes)) {
        //place here
        loc->status = 1;
        backtrace(loc->trace, 3);
        return (void*) (loc + 1);
      }
      loc = next_block(loc);
    }
    //if no free space in heap, extend
    struct header* newHeader = ((struct header*) sbrk(nbytes + 48));
    if(newHeader) {
      newHeader->payload_size = nbytes;
      newHeader->status = 1;
      backtrace(newHeader->trace, 3);
      memcpy(newHeader->redzone, "107e", 4);
      memcpy(far_redzone(newHeader), "107e", 4);
      return newHeader + 1;
    }
    return NULL;
}

void free (void *ptr)
{
    free_calls++;
    if(ptr) {
      struct header* this_block = ((struct header*) ptr - 1);
      this_block->status = 0;
      //check redzones
      char first_rz[5];
      memset(first_rz, 0, 5);
      memcpy(first_rz, this_block->redzone, 4);
      char second_rz[5];
      memset(second_rz, 0, 5);
      memcpy(second_rz, far_redzone(this_block), 4);
      if((strcmp(first_rz, "107e") != 0) || (strcmp(second_rz, "107e") != 0)) {
        printf("============================================= \n");
        printf("**********  Mini-Valgrind Alert  ********** \n");
        printf("============================================= \n");
        printf("Attempt to free adress %p that has damaged red zone(s): [%s] [%s] \n", ptr,
         first_rz, second_rz);
         printf("Block of size %d bytes, allocated by\n", this_block->payload_size);
         print_frames(this_block->trace, 3);
      }
      //try to combine freespace
      struct header* check_block = next_block(this_block);
      while(((char*) check_block < (char*) heap_end) && (check_block->status == 0)) {
          this_block->payload_size += check_block->payload_size + 48;
          check_block = next_block(this_block);
      }
    }
}

void *realloc (void *orig_ptr, size_t new_size)
{
    //special cases
    if(new_size == 0) {
      free(orig_ptr);
      return NULL;
    }
    if(!orig_ptr) {
      return malloc(new_size);
    }
    struct header* this_block = ((struct header*) orig_ptr - 1);
    new_size = roundup(new_size, 8);
    if(new_size < this_block->payload_size) {
      return orig_ptr;
    }
    //try in place resize
    struct header* block = this_block;
    block->status = 0;
    int potential_size = 0;
    while((block < (struct header*)heap_end) && !block->status) {
      potential_size += block->payload_size + 48;
      if(new_size < potential_size) {
        this_block->status = 1;
        this_block->payload_size = potential_size;
        return orig_ptr;
      }
      block = next_block(block);
    }
    //if that fails, allocate new memory
    void *new_ptr = malloc(new_size);
    if(!new_ptr)
      return NULL;
    memcpy(new_ptr, orig_ptr, this_block->payload_size);
    free(orig_ptr);
    return new_ptr;
}

void heap_dump (const char *label)
{
  printf("\n---------- HEAP DUMP (%s) ----------\n", label);
  printf("Heap segment at %p - %p\n", heap_start, heap_end);
  for(struct header* loc = (struct header*) heap_start; loc < (struct header*) heap_end;
      loc = next_block(loc))
  {
        printf("blocksize=%d   status=%d\n", loc->payload_size, loc->status);
        if(loc->payload_size == 0) {
          break;
        }
  }
  printf("----------  END DUMP (%s) ----------\n", label);
}

char* far_redzone(struct header* block) {
  return (char*) next_block(block) - 4;
}

struct header* next_block(struct header* block) {
  return  (struct header*) ((char*) block + block->payload_size + 48);
}

void memory_report (void)
{
    printf("\n=============================================\n");
    printf(  "         Mini-Valgrind Memory Report         \n");
    printf(  "=============================================\n");
    printf("malloc/free: %d allocs, %d frees, %d bytes allocated\n", malloc_calls,
      free_calls, bytes_allocated);
    struct header* check_block = heap_start;
    while(((char*)check_block < (char*)heap_end)) {
      if(check_block->status) {
        //unfreed memory
        printf("%d bytes are lost, allocated by\n", check_block->payload_size);
        print_frames(check_block->trace, 3);
        printf("\n");
      }
      check_block = next_block(check_block);
    }
  }
