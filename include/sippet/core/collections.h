#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Represents a dynamic resizable array.
 */
struct SippetArray {
  /**
   * The data stored in the `SippetArray`.
   */
  void **data;

  /**
   * The number of elements in the `void **SippetArray::data`.
   */
  size_t length;

  /**
   * The maximum number of elements `void **SippetArray::data` can hold.
   */
  size_t capacity;

  /**
   * The size of each element in `void **SippetArray::data`.
   */
  size_t size;
};

/**
 * Allocates memory for a `SippetArray` given its capacity and element size.
 */
struct SippetArray *sippetCreateArray(const size_t capacity, const size_t size);

/**
 * Pushes some data into a `SippetArray`.
 */
bool sippetPushArray(const void *const data, const size_t count, struct SippetArray *const array);

/**
 * Frees the memory of a `SippetArray`.
 */
void sippetDestroyArray(struct SippetArray *const array);

/**
 * Represents an entry in the hash table, storing a key-value pair. Uses chaining (linked list) to
 * handle collisions.
 */
struct SippetTableEntry {
  char *key;
  void *value;
  struct SippetTableEntry *next;
};

/**
 * Represents a hash table with an array of buckets (linked lists). `size_t SippetTable::count`
 * keeps track of the number of stored elements, and `size_t SippetTable::size` is the bucket array
 * size.
 */
struct SippetTable {
  struct SippetTableEntry **buckets;
  size_t count;
  size_t size;
};

/**
 * Creates and initializes a new hash table. Returns a pointer to the table or `NULL` if allocation
 * fails.
 */
struct SippetTable *sippetCreateTable();

/**
 * Retrieves the value with a given key in the hash table. Returns `NULL` if the key is not found.
 */
void *sippetGetTable(const struct SippetTable *const table, const char *key);

/**
 * Inserts a or updates key-value pair into the hash table.
 */
bool sippetSetTable(struct SippetTable *const table, const char *key, void *value);

/**
 * Frees all memory associated with the hash table, including keys, values, and entries. If a free
 * function other than `NULL` is provided, it will be used to free each stored value.
 */
void sippetDestroyTable(struct SippetTable *const table, void (*const freeValueFunction)(void *));