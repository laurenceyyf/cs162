#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Function pointers to hw3 functions */
void* (*mm_malloc)(size_t);
void* (*mm_realloc)(void*, size_t);
void (*mm_free)(void*);

static void* try_dlsym(void* handle, const char* symbol) {
  char* error;
  void* function = dlsym(handle, symbol);
  if ((error = dlerror())) {
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
  }
  return function;
}

static void load_alloc_functions() {
  void* handle = dlopen("hw3lib.so", RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  mm_malloc = try_dlsym(handle, "mm_malloc");
  mm_realloc = try_dlsym(handle, "mm_realloc");
  mm_free = try_dlsym(handle, "mm_free");
}

int main() {
  load_alloc_functions();

  int* data1 = mm_malloc(100);
  assert(data1 != NULL);
  data1[0] = 0x162;
  printf("%p\n", data1);
  printf("%x\n", data1[0]);
  int* data2 = mm_malloc(100);
  assert(data2 != NULL);
  printf("%p\n", data2);
  data1 = mm_realloc(data1, 200);
  assert(data1 != NULL);
  printf("%p\n", data1);
  printf("%x\n", data1[0]);
  int* data3 = mm_malloc(100);
  assert(data3 != NULL);
  printf("%p\n", data3);
  int* data4 = mm_realloc(data1, SIZE_MAX);
  printf("%p\n", data4);
  printf("%p\n", data1);
  printf("%x\n", data1[0]);
  mm_free(data1);
  mm_free(data2);
  mm_free(data3);
  mm_free(data4);
  puts("malloc test successful!");
}
