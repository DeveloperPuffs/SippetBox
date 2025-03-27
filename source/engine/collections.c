#include <stdlib.h>
#include <string.h>
#include <engine/logging.h>
#include <engine/collections.h>

struct List *createList(const unsigned long capacity, const unsigned long size) {
  if (!size) return (logError("Invalid size for list"), NULL);

  struct List *const list = malloc(sizeof(struct List));
  if (!list) return NULL;

  list->length = 0, list->capacity = capacity, list->size = size;
  return list->capacity ? (list->data = malloc(capacity * size)) ? list : (free(list), NULL) : list;
}

unsigned char pushList(const void *const data, const unsigned long count, struct List *const list) {
  if (!count) return 1;

  if (!data) return (logError("Cannot push invalid data to list"), 0);
  if (!list) return (logError("Cannot push data to invalid list"), 0);

  if (list->length + count > list->capacity) {
    list->capacity = 2 * (list->capacity + count);
    void *reallocatedList = realloc(list->data, list->capacity * list->size);
    if (!reallocatedList) return 0;
    *list->data = reallocatedList;
  }

  memcpy((char *)list->data + list->length * list->size, data, count * list->size);
  return (list->length += count, 1);
}

void destroyList(struct List *const list) {
  if (!list) return;
  free(list->data);
  free(list);
}