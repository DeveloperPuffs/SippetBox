#pragma once

/**
 * Stores data about a bitmap image, captured at a specific time.
 */
struct Image {
  /**
   * The width of the bitmap image in pixels.
   */
  unsigned int width;

  /**
   * The height of the bitmap image in pixels.
   */
  unsigned int height;

  /**
   * The number of color channels (3 for RGB, 4 for RGBA).
   */
  unsigned char channels;

  /**
   * The number of bits per channel.
   */
  unsigned char depth;

  /**
   * The data of the bitmap image.
   */
  unsigned char **data;
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
  unsigned long size;

  /**
   * The binary data of the shader code.
   */
  unsigned int *data;
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
   * The coordinates of the vertices of the model as a flat array of floats where each three
   * consecutive floats represent one vertex (`{x, y, z, x, y, z, x, y, z, ...}`).
   */
  float *vertices;

  /**
   * The number of coordinates/elements in `float *Object::vertices`.
   */
  unsigned long vertexCount;

  /**
   * The maximum number of coordinates/elements `float *Object::vertices` can hold.
   */
  unsigned long vertexCapacity;

  /**
   * The indices of the model, representing the faces of the model. Stored in a flat array of
   * unsigned integers.
   */
  unsigned long *indices;

  /**
   * The number of individual indices in `unsigned int *Object::indices`.
   */
  unsigned long indexCount;

  /**
   * The maximum number of indices `unsigned int *Object::indices` can hold.
   */
  unsigned long indexCapacity;

  /**
   * The coordinates of the normals of the model as a flat array of floats where each three
   * consecutive floats represent one normal (`{x, y, z, x, y, z, x, y, z, ...}`).
   */
  float *normals;

  /**
   * The number of coordinates/elements in `float *Object::normals`.
   */
  unsigned long normalCount;

  /**
   * The maximum number of coordinates/elements `float *Object::normals` can hold.
   */
  unsigned long normalCapacity;

  /**
   * The coordinates of the texture of the model as a flat array of floats where each three
   * consecutive floats represent one vertex (`{u, v, w, u, v, w, u, v, w, ...}`).
   */
  float *textureCoordinates;

  /**
   * The number of coordinates/elements in `float *Object::textureCoordinates`.
   */
  unsigned long textureCoordinateCount;

  /**
   * The maximum number of coordinates/elements `float *Object::textureCoordinates` can hold.
   */
  unsigned long textureCoordinateCapacity;

  /**
   * The names of the material files (`.mtl`) used by the model.
   */
  char **const materialFiles;

  /**
   * The number of material files used by the model.
   */
  unsigned long materialFileCount;

  /**
   * The total number of material file names `char **const Object::materialFiles` can hold.
   */
  unsigned long materialFileCapacity;
};

/**
 * Creates an instance of `Object` and initializes its resources.
 */
struct Object *createObject(const char *const path);

/**
 * Destroys an instance of `Object` by freeing its resources.
 */
void destroyObject(struct Object *const object);