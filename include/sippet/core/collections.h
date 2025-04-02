#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Represents a dynamic array (resizable).
 */
struct Array {
  /**
   * The data stored in the `Array`.
   */
  void **data;

  /**
   * The number of elements in the `void **Array::data`.
   */
  size_t length;

  /**
   * The maximum number of elements `void **Array::data` can hold.
   */
  size_t capacity;

  /**
   * The size of each element in `void **Array::data`.
   */
  size_t size;
};

/**
 * Allocates memory for a `Array` given its capacity and element size.
 */
struct Array *createArray(const size_t capacity, const size_t size);

/**
 * Pushes some data into a `Array`.
 */
bool pushArray(const void *const data, const size_t count, struct Array *const array);

/**
 * Frees the memory of a `Array`.
 */
void destroyArray(struct Array *const array);

/**
 * Represents an entry in the hash table, storing a key-value pair. Uses chaining (linked list) to
 * handle collisions.
 */
struct TableEntry {
  char *key;
  void *value;
  struct TableEntry *next;
};

/**
 * Represents a hash table with an array of buckets (linked lists). `unsigned long Table::count`
 * keeps track of the number of stored elements, and `unsigned long Table::size` is the bucket
 * array size.
 */
struct Table {
  struct TableEntry **buckets;
  size_t count;
  size_t size;
};

/**
 * Creates and initializes a new hash table. Returns a pointer to the table or `NULL` if allocation
 * fails.
 */
struct Table *createTable();

/**
 * Retrieves the value with a given key in the hash table. Returns `NULL` if the key is not found.
 */
void *getTable(const struct Table *const table, const char *key);

/**
 * Inserts a or updates key-value pair into the hash table. Returns `1` for success and `0` for
 * failure.
 */
bool setTable(struct Table *const table, const char *key, void *value);

/**
 * Frees all memory associated with the hash table, including keys, values, and entries. If a free
 * function other than `NULL` is provided, it will be used to free each stored value.
 */
void destroyTable(struct Table *const table, void (*const freeValueFunction)(void *));