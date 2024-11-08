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
    memset((void*)new_meta + sizeof(struct mm_meta), 0, size);
    return (void*)new_meta + sizeof(struct mm_meta);
  } else {
    struct mm_meta* cur = head;
    while (cur != NULL) {
      if (cur->size >= size && cur->free == 1) {
        size_t unused = cur->size - size;
        if (unused <= sizeof(struct mm_meta)) {
          cur->size = size;
          cur->unused = unused;
          cur->free = 0;
          memset((void*)cur + sizeof(struct mm_meta), 0, size + unused);
        } else {
          struct mm_meta* new_meta = (struct mm_meta*)((void*)cur + size + sizeof(struct mm_meta));
          new_meta->size = cur->size - sizeof(struct mm_meta);
          new_meta->unused = 0;
          new_meta->free = 1;
          new_meta->prev = cur;
          new_meta->next = cur->next;
          if (cur != tail) {
            new_meta->next->prev = new_meta;
          } else {
            tail = new_meta;
          }
          memset((void*)new_meta + sizeof(struct mm_meta), 0, size);

          cur->size = size;
          cur->unused = 0;
          cur->free = 0;
          cur->next = new_meta;
          memset((void*)cur + sizeof(struct mm_meta), 0, size);
        }
        return (void*)((void*)cur + sizeof(struct mm_meta));
      } else if (cur == tail) {
        void* vaddr = sbrk(size + sizeof(struct mm_meta));
        if (vaddr == (void*)-1) {
          return NULL;
        }
        struct mm_meta* new_meta = (struct mm_meta*)vaddr;
        tail = new_meta;
        new_meta->size = size;
        new_meta->free = 0;
        new_meta->next = NULL;
        new_meta->prev = cur;
        cur->next = new_meta;
        memset((void*)new_meta + sizeof(struct mm_meta), 0, size);
        return (void*)new_meta + sizeof(struct mm_meta);
      } else {
        cur = cur->next;
      }
    }
  }
  return NULL;
}

void* mm_realloc(void* ptr, size_t size) {
  //TODO: Implement realloc
  if (ptr == NULL) {
    return NULL;
  }
  return (void*)(ptr+size);
}

void mm_free(void* ptr) {
  if (ptr == NULL) {
    return;
  }
  struct mm_meta* cur = (struct mm_meta*)(ptr - sizeof(struct mm_meta));
  cur->free = 1;
  cur->size += cur->unused;
  cur->unused = 0;
  if (cur != tail && cur->next->free == 1) {
    cur->size += (cur->next->size + sizeof(struct mm_meta));
    if (cur->next == tail) {
      tail = cur;
    } else {
      cur->next = cur->next->next;
      cur->next->prev = cur;
    }
  } else if (cur != head && cur->prev->free == 1) {
    cur->prev->size += (cur->size + sizeof(struct mm_meta));
    if (cur == tail) {
      tail = cur->prev;
    } else {
      cur->prev->next = cur->next;
      cur->next->prev = cur->prev;
    }
  }
  return;
}
