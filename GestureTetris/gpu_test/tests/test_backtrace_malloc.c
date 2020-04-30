#include "assert.h"
#include "backtrace.h"
#include "malloc.h"
#include "../malloc_internal.h"
#include "../nameless.h"
#include "printf.h"
#include "rand.h"
#include <stdint.h>
#include "strings.h"
#include "uart.h"

void main(void);

//simple list datatype
struct list {
  struct list* next;
  void* data;
};

struct list* create_node(void* data) {
  struct list* node = (struct list*) malloc(8);
  node->data = data;
  node->next = NULL;
  return node;
}

void append(struct list* node, void* data) {
  struct list* end_node = node;
  while(end_node->next) {
    end_node = end_node->next;
  }
  struct list* to_append = create_node(data);
  end_node->next = to_append;
}

//inteprets data as strings
char* print_list(struct list* node) {
  char* result = malloc(1024);
  if(!node->next) {
    strlcat(result, node->data, 1024);
    return result;
  }
  while(node->next) {
     strlcat(result, node->data, 1024);
     node = node->next;
  }
  strlcat(result, node->data, 1024);
  return result;
}

static void test_name_of(void)
{
    const char *name;
    name = name_of((uintptr_t)main);
    printf("%s\n", name);
    assert(strcmp(name, "main") == 0);
    name = name_of((uintptr_t)uart_init);
    assert(strcmp(name, "uart_init") == 0);
    name = name_of((uintptr_t)mystery); // function compiled without embedded name
    assert(strcmp(name, "???") == 0);
}

static void test_backtrace_simple(void)
{
    frame_t f[2];
    int frames_filled = backtrace(f, 2);

    assert(frames_filled == 2);
    printf("name: %s\n",f[0].name );
    assert(strcmp(f[0].name, "test_backtrace_simple") == 0);
    printf("my resume addr%x\n", f[0].resume_addr);
    printf("my resume offset%x\n", f[0].resume_offset);
    printf("function start%x\n", (uintptr_t)test_backtrace_simple);
    assert(f[0].resume_addr == (uintptr_t)test_backtrace_simple + f[0].resume_offset);
    assert(strcmp(f[1].name, "main") == 0);
    assert(f[1].resume_addr == (uintptr_t)main + f[1].resume_offset);
    printf("Here is a simple backtrace:\n");
    print_frames(f, frames_filled);
    printf("\n");
}

static int recursion_fun(int n)
{
    if (n == 0)
        return mystery();   // look in nameless.c
    else
        return 1 + recursion_fun(n-1);
}

static int test_backtrace_complex(int n)
{
    return recursion_fun(n);
}

static void test_heap_dump(void)
{
    //test more advanced malloc_internal
    char* g = malloc(50);
    free(g);
    char* s = malloc(30);

    heap_dump("My heap");
    free(s);

    char *p = malloc(24);
    heap_dump("After p = malloc(24)");

    free(p);
    heap_dump("After free(p)");

    p = malloc(16);
    heap_dump("After p = malloc(16)");

    p = realloc(p, 32);
    heap_dump("After p = realloc(p, 32)");

    free(p);
    heap_dump("After free(p)");
}

static void test_heap_simple(void)
{
    char *s;
    s = malloc(6);
    memcpy(s, "hello", 6);
    assert(strcmp(s, "hello") == 0);

    heap_dump("simple");

    s = realloc(s, 12);
    strlcat(s, " world", 12);
    assert(strcmp(s, "hello world") == 0);
    free(s);
}

// array of dynamically-allocated strings, each
// string filled with repeated char, e.g. "A" , "BB" , "CCC"
// Examine each string, verify expected contents intact.
static void test_heap_multiple(void)
{
    int max_trials = 3;
    char *arr[max_trials*8];

    for (int ntrials = 1; ntrials <= max_trials; ntrials++) {
        int n = (ntrials*8);
        for (int i = 0; i < n; i++) {
            int num_repeats = i + 1;
            char *ptr = malloc(num_repeats + 1);
            assert(ptr != NULL);
            assert((uintptr_t)ptr % 8 == 0); // verify 8-byte alignment
            memset(ptr, 'A' - 1 + num_repeats, num_repeats);
            ptr[num_repeats] = '\0';
            arr[i] = ptr;
        }
        heap_dump("After all allocations");
        for (int i = n-1; i >= 0; i--) {
            int len = strlen(arr[i]);
            char first = arr[i][0], last = arr[i][len -1];
            assert(first == 'A' - 1 + len);  // verify payload contents
            assert(first == last);
            free(arr[i]);
        }
        heap_dump("After all frees");
    }
}

#define max(x, y) ((x) > (y) ? (x) : (y))

static void test_heap_recycle(int niter)
{
    extern int __bss_end__;
    void *heap_low = &__bss_end__, *heap_high = NULL;

    size_t max_size = 1024, total = 0;
    void *p = malloc(1);

    for (int i = 0; i < niter; i++) {
        size_t size = rand() % max_size;
        void *q = malloc(size);
        p = realloc(p, size);
        total += 2*size;
        void *higher = (char *)max(p, q) + size;
        heap_high = max(heap_high, higher);
        free(q);
    }
    free(p);
    size_t extent = (char *)heap_high - (char *)heap_low;
    size_t percent = total > extent ? (100*total)/extent : 0;
    printf("\nRecycling report for %d iterations:\n", niter);
    printf("Serviced requests totaling %d bytes, heap extent is %d bytes. Recycled %d%%\n", total, extent, percent);
}

void test_heap_redzones(void)
{

    // heap_dump("check heap");
    // DO NOT ATTEMPT THIS TEST unless your heap has red zone protection!
    char *ptr;

    ptr = malloc(9);
    memset(ptr, 'a', 9); // write into payload
    free(ptr); // ptr is OK

    ptr = malloc(5);
    ptr[-1] = 0x45; // write before payload
    free(ptr);      // ptr is NOT ok

    ptr = malloc(12);
    ptr[13] = 0x45; // write after payload
    free(ptr);      // ptr is NOT ok
}

static void test_heap_odd(void) {
  assert(malloc(0) == NULL);
  assert(realloc(NULL, 0) == NULL);
  char* q = realloc(NULL, 8);
  assert(q != NULL);
  free(q);
  char* p = malloc(20);
  assert(realloc(p, 0) == NULL);
}

static void test_heap_myrealloc(void) {
  //runs on clear heap
  char* a = malloc(50);
  char* b = malloc(30);
  char* c = malloc(30);
  free(a);
  free(b);
  char* d = malloc(40);
  assert(d < c);
  d = realloc(d, 200);
  assert(d > c);
  free(c);
  free(d);
}

static void test_heap_datacopy(void) {
  //runs on clear heap
  char* a = malloc(50);
  char* b = malloc(30);
  char* c = malloc(30);
  snprintf(a, 50, "abc");
  assert(strcmp(a, "abc") == 0);
  heap_dump("before realloc a 1");
  a = realloc(a, 90);
  assert(strcmp(a, "abc") == 0);
  heap_dump("before realloc a 2");
  a = realloc(a, 60);
  assert(strcmp(a, "abc") == 0);
  free(b);
  free(c);
  a = realloc(a, 50);
  free(a);
  assert(strcmp(a, "abc") == 0);
}

static void test_heap_merge(void) {
  //runs on clear heap
  char* a = malloc(50);
  char* b = malloc(30);
  char* c = malloc(30);
  free(c);
  free(b);
  free(a);
  char* d = malloc(60);
  assert(d < c);
  free(d);
  heap_dump("my test: merge");
}

static void test_heap_bad(void) {
  char* a = malloc(30);
  char* b = malloc(30);
  char* c = malloc(30);
  a = realloc(a, 40);
  free(c);
  free(b);
  char* d = malloc(20);
  free(d);
  free(a);
  assert(d < c);
}


static void test_heap_list(void) {
  //leaks lots of memory
  heap_dump("pre list");
  char* a = malloc(30);
  char* b = malloc(30);
  char* c = malloc(30);
  memcpy(a, "hello", 6);
  memcpy(b, ", this ", 8);
  memcpy(c, "is a list", 10);
  heap_dump("list data");
  struct list* node = create_node((void*) a);
  append(node, (void*) b);
  append(node, (void*) c);
  heap_dump("form list");
  printf("test list: %s\n", print_list(node));
  assert(strcmp(print_list(node), "hello, this is a list") == 0);
  //test realloc
  node->next->data = realloc(node->next->data, 70);
  node->next->next->data = realloc(node->next->next->data, 70);
  assert(strcmp(print_list(node), "hello, this is a list") == 0);
  node->next = realloc(node->next, 90);
  assert(strcmp(print_list(node), "hello, this is a list") == 0);
  heap_dump("after list");
}

void my_test_redzone() {
  char* a = malloc(5);
  memset(a, 0, 8);
  char* b = malloc(6);
  snprintf(b, 10, "hello world");
  char* c = malloc(20);
  memset(c, 0, 1);
  strlcat(c, b,34);
  strlcat(c, b, 40);
  strlcat(c, b, 40);
  strlcat(c, b, 50);
  free(a);
  free(b);
  free(c);
}

void main(void)
{
    uart_init();
    uart_putstring("Start execute main() in tests/test_backtrace_malloc.c\n");

    test_name_of();

    test_backtrace_simple();
    test_backtrace_simple();
    // Again so you can see the main offset change!
    test_backtrace_complex(7);  // Slightly tricky backtrace

    //my tests
    // test_heap_list();
    test_heap_myrealloc();
    test_heap_odd();
    heap_dump("before datacopy");
    test_heap_datacopy();
    test_heap_merge();
    test_heap_bad();

    test_heap_dump();

    test_heap_simple();
    test_heap_simple();

    test_heap_multiple();
    test_heap_recycle(10);

    test_heap_dump();

    test_heap_simple();
    test_heap_multiple();
    test_heap_recycle(20);
    heap_dump("after bad");
    test_heap_redzones(); // DO NOT USE unless you implemented red zone protection
    my_test_redzone();
    uart_putstring("\nSuccessfully finished executing main() in tests/test_backtrace_malloc.c\n");
}
