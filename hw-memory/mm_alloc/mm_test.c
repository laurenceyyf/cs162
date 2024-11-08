#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

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
  int* data2 = mm_malloc(150);
  assert(data2 != NULL);
  int* data3 = mm_malloc(200);
  assert(data3 != NULL);
  int* data4 = mm_malloc(300);
  int* data5 = mm_malloc(400);
  int* data6 = mm_malloc(500);
  int* data7 = mm_malloc(600);
  int* data8 = mm_malloc(700);
  mm_free(data1);
  mm_free(data2);
  mm_free(data3);
  mm_free(data4);
  mm_free(data5);
  mm_free(data6);
  mm_free(data7);
  mm_free(data8);
  int* data = mm_malloc(2950);
  mm_free(data);
  puts("malloc test successful!");
}
