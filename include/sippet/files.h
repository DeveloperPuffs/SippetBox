#pragma once
#include <stdint.h>
#include <sippet/core/collections.h>

/**
 * Stores data about a bitmap image, captured at a specific time.
 */
struct Image {
  /**
   * The width of the bitmap image in pixels.
   */
  size_t width;

  /**
   * The height of the bitmap image in pixels.
   */
  size_t height;

  /**
   * The number of color channels (3 for RGB, 4 for RGBA).
   */
  uint8_t channels;

  /**
   * The number of bits per channel.
   */
  uint8_t depth;

  /**
   * The data of the bitmap image.
   */
  uint8_t **data;
};

/**
 * Creates an instance of `Image` and initializes its resources.
 */
struct Image *createImage(const char *const path);

/**
 * Destroys an instance of `Image` by freeing its resources.
 */
void destroyImage(struct Image *const image);

/**
 * Stores data about a SPIR-V shader file.
 */
struct Shader {
  /**
   * The size of the shader code in bytes.
   */
  size_t size;

  /**
   * The binary data of the shader code.
   */
  uint32_t *data;
};

/**
 * Creates an instance of `Shader` and initializes its resources.
 */
struct Shader *createShader(const char *const path);

/**
 * Destroys an instance of `Shader` by freeing its resources.
 */
void destroyShader(struct Shader *const shader);

/**
 * Stores data about an `.obj` 3D model.
 */
struct Object {
  /**
   * The positions parsed from the object as a flat array of `float`s (`x, y, z`).
   */
  struct Array *positions;

  /**
   * The normals parsed or computed from the object as a flat array of `float`s (`i, j, k`).
   */
  struct Array *normals;

  /**
   * The texture coordinates parsed from the object as a flat array of `float`s (`u, v, w`).
   */
  struct Array *textureCoordinates;

  /**
   * The indices parsed from the object as a flat array of `uint32_t`s (`positions[0],
   * positions[1], positions[2], normals[0], normals[1], textureCoordinates[0],
   * textureCoordinates[1], textureCoordinates[2]`).
   */
  struct Array *indices;
};

/**
 * Creates an instance of `Object` and initializes its resources.
 */
struct Object *createObject(const char *const path);

/**
 * Destroys an instance of `Object` by freeing its resources.
 */
void destroyObject(struct Object *const object);