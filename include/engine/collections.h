#pragma once

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
  unsigned long length;

  /**
   * The maximum number of elements `void **Array::data` can hold.
   */
  unsigned long capacity;

  /**
   * The size of each element in `void **Array::data`.
   */
  unsigned long size;
};

/**
 * Allocates memory for a `Array` given its capacity and element size.
 */
struct Array *createArray(const unsigned long capacity, const unsigned long size);

/**
 * Pushes some data into a `Array`.
 */
unsigned char pushArray(const void *const data, const unsigned long count, struct Array *const array);

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
  unsigned long count;
  unsigned long size;
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
unsigned char setTable(struct Table *const table, const char *key, void *value);

/**
 * Frees all memory associated with the hash table, including keys, values, and entries. If a free
 * function other than `NULL` is provided, it will be used to free each stored value.
 */
void destroyTable(struct Table *const table, void (*const freeValueFunction)(void *));