#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <engine/logging.h>
#include <engine/collections.h>

struct Array *createArray(const size_t capacity, const size_t size) {
  if (!size) return (logError("Invalid size for array"), NULL);

  struct Array *const array = malloc(sizeof(struct Array));
  if (!array) return NULL;

  array->length = 0, array->capacity = capacity, array->size = size;
  return array->capacity ? (array->data = malloc(capacity * size)) ? array : (free(array), NULL) : array;
}

bool pushArray(const void *const data, const size_t count, struct Array *const array) {
  if (!count) return 1;

  if (!data) return (logError("Cannot push invalid data to array"), 0);
  if (!array) return (logError("Cannot push data to invalid array"), 0);

  if (array->length + count > array->capacity) {
    array->capacity = 2 * (array->capacity + count);
    void *reallocatedArray = realloc(array->data, array->capacity * array->size);
    if (!reallocatedArray) return 0;
    array->data = reallocatedArray;
  }

  memcpy((char *)array->data + array->length * array->size, data, count * array->size);
  array->length += count;
  return 1;
}

void destroyArray(struct Array *const array) {
  if (!array) return;
  free(array->data);
  free(array);
}

#define INITIAL_TABLE_SIZE 101
#define TABLE_LOAD_FACTOR 0.7
#define HASH_MULTIPLIER 5381

struct Table *createTable() {
  struct Table *const table = malloc(sizeof(struct Table));
  if (!table) return NULL;

  if (!(table->buckets = calloc(INITIAL_TABLE_SIZE, sizeof(struct TableEntry *)))) return (free(table), NULL);
  table->size = INITIAL_TABLE_SIZE;
  table->count = 0;
  return table;
}

static size_t hashFunction(const char *key, const size_t size) {
  size_t hash = HASH_MULTIPLIER;
  while (*key) hash = ((hash << 5) + hash) + *key++;
  return hash % size;
}

void *getTable(const struct Table *const table, const char *key) {
  const size_t index = hashFunction(key, table->size);
  struct TableEntry *entry = table->buckets[index];
  while (entry) {
    if (!strcmp(entry->key, key)) return entry->value;
    entry = entry->next;
  }

  return NULL;
}

static bool resizeTable(struct Table *const table) {
  const size_t newSize = 2 * table->size;
  struct TableEntry **newBuckets = calloc(newSize, sizeof(struct TableEntry *));
  if (!newBuckets) return 0;

  for (size_t i = 0; i < table->size; i++) {
      struct TableEntry *entry = table->buckets[i];
      while (entry) {
          struct TableEntry *next = entry->next;

          const size_t newIndex = hashFunction(entry->key, newSize);
          entry->next = newBuckets[newIndex];
          newBuckets[newIndex] = entry;

          entry = next;
      }
  }

  free(table->buckets);
  table->buckets = newBuckets;
  table->size = newSize;
  return 1;
}

bool setTable(struct Table *const table, const char *key, void *value) {
  if ((double)table->count / table->size > TABLE_LOAD_FACTOR && !resizeTable(table)) return 0;

  const size_t index = hashFunction(key, table->size);
  struct TableEntry *entry = table->buckets[index];
  while (entry) {
    if (!strcmp(entry->key, key)) return (entry->value = value, 1);
    entry = entry->next;
  }

  struct TableEntry *const nextEntry = malloc(sizeof(struct TableEntry));
  if (!nextEntry) return 0;

  if (!(nextEntry->key = strdup(key))) return (free(nextEntry), 0);
  nextEntry->value = value;
  nextEntry->next = table->buckets[index];
  table->buckets[index] = nextEntry;
  return (++table->count, 1);
}

void destroyTable(struct Table *const table, void (*const freeValueFunction)(void *)) {
  if (!table) return;
  for (size_t i = 0; i < table->size; ++i) {
    struct TableEntry *entry = table->buckets[i];
    while (entry) {
      struct TableEntry *nextEntry = entry->next;
      if (freeValueFunction) freeValueFunction(entry->value);
      free(entry->key);
      free(entry);

      entry = nextEntry;
    }
  }

  free(table->buckets);
  free(table);
}