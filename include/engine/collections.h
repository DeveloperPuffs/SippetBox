#pragma once

/**
 * Represents a dynamic array (resizable)
 */
struct List {
  /**
   * The data stored in the `List`
   */
  void **data;

  /**
   * The number of elements in the `void **List::data`
   */
  unsigned long length;

  /**
   * The maximum number of elements `void **List::data` can hold
   */
  unsigned long capacity;

  /**
   * The size of each element in `void **List::data`
   */
  unsigned long size;
};

/**
 * Allocates memory for a `List` given its capacity and element size
 */
struct List *createList(const unsigned long capacity, const unsigned long size);

/**
 * Pushes some data into a `List`
 */
unsigned char pushList(const void *const data, const unsigned long count, struct List *const list);

/**
 * Frees the memory of a `List`
 */
void destroyList(struct List *const list);

// TODO: Implement a hash table