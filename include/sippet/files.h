#pragma once
#include <stdint.h>
#include <sippet/core/collections.h>

/**
 * Stores data about a bitmap image, captured at a specific time.
 */
struct SippetImage {
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
 * Creates an instance of `SippetImage` and initializes its resources.
 */
struct SippetImage *sippetCreateImage(const char *const path);

/**
 * Destroys an instance of `SippetImage` by freeing its resources.
 */
void sippetDestroyImage(struct SippetImage *const image);

/**
 * Stores data about a SPIR-V shader file.
 */
struct SippetShader {
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
 * Creates an instance of `SippetShader` and initializes its resources.
 */
struct SippetShader *sippetCreateShader(const char *const path);

/**
 * Destroys an instance of `SippetShader` by freeing its resources.
 */
void sippetDestroyShader(struct SippetShader *const shader);

/**
 * Stores data about an `.obj` 3D model.
 */
struct SippetObject {
  /**
   * The positions parsed from the object as a flat array of `float`s (`x, y, z`).
   */
  struct SippetArray *positions;

  /**
   * The normals parsed or computed from the object as a flat array of `float`s (`i, j, k`).
   */
  struct SippetArray *normals;

  /**
   * The texture coordinates parsed from the object as a flat array of `float`s (`u, v, w`).
   */
  struct SippetArray *textureCoordinates;

  /**
   * The indices parsed from the object as a flat array of `uint32_t`s (`positions[0],
   * positions[1], positions[2], normals[0], normals[1], textureCoordinates[0],
   * textureCoordinates[1], textureCoordinates[2]`).
   */
  struct SippetArray *indices;
};

/**
 * Creates an instance of `SippetObject` and initializes its resources.
 */
struct SippetObject *sippetCreateObject(const char *const path);

/**
 * Destroys an instance of `SippetObject` by freeing its resources.
 */
void sippetDestroyObject(struct SippetObject *const object);