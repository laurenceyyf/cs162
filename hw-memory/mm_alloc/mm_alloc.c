/*
 * mm_alloc.c
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

struct mm_meta* head = NULL;
struct mm_meta* tail = NULL;

void* mm_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  if (head == NULL) {
    void* vaddr = sbrk(size + sizeof(struct mm_meta));
    if (vaddr == (void*)-1) {
      return NULL;
    }
    struct mm_meta* new_meta = (struct mm_meta*)vaddr;
    head = new_meta;
    tail = new_meta;
    new_meta->size = size;
    new_meta->free = 0;
    new_meta->next = NULL;
    new_meta->prev = NULL;
  } else {
    struct mm_meta* cur = head;
    while (cur != NULL) {
      if (cur->size >= size + sizeof(struct mm_meta) && cur->free == 1) {
        size_t unused = cur->size - size - sizeof(struct mm_meta);
        if (unused <= sizeof(struct mm_meta)) {
          cur->size = size;
          cur->unused = unused;
          cur->free = 0;
          memset(cur + sizeof(struct mm_meta), 0, size + unused);
        } else {
          struct mm_meta* new_meta = (struct mm_meta*)(cur + cur->size + sizeof(struct mm_meta));
          new_meta->size = cur->size - sizeof(struct mm_meta);
          new_meta->unused = 0;
          new_meta->free = 1;
          new_meta->prev = cur;
          new_meta->next = cur->next;
          memset(new_meta + sizeof(struct mm_meta), 0, size);

          cur->size = size;
          cur->unused = 0;
          cur->free = 0;
          if (cur != tail) {
            cur->next->next->prev = new_meta;
          } else {
            tail = new_meta;
          }
          cur->next = new_meta;
          memset(cur + sizeof(struct mm_meta), 0, size);
        }
        return (void*)(cur + sizeof(struct mm_meta));
      } else {
        cur = cur->next;
      }
    }
  }
  return NULL;
}

void* mm_realloc(void* ptr, size_t size) {
  //TODO: Implement realloc
  ptr = NULL;
  size = 0;
  return (void*)(ptr+size);
}

void mm_free(void* ptr) {
  //TODO: Implement free
  ptr = NULL;
  if (ptr == NULL) return;
}
