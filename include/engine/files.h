#pragma once

/**
 * Stores data about a bitmap image, captured at a specific time.
 */
struct Image
{
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

struct Image *image_create(const char *const path);

/**
 * Destroys an instance of `Image` by freeing its resources.
 */
void image_destroy(struct Image *const image);

/**
 * Stores data about a SPIR-V shader file.
 */
struct Shader
{
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
struct Shader *shader_create(const char *const path);

/**
 * Destroys an instance of `Shader` by freeing its resources.
 */
void shader_destroy(struct Shader *const shader);

/**
 * Stores data about an `.obj` 3D model.
 */
struct Object
{
    float *vertices;
    unsigned int vertex_count;
    unsigned int *indices;
    unsigned int index_count;
};

/**
 * Creates an instance of `Object` and initializes its resources.
 */
struct Object *object_create(const char *const path);

/**
 * Destroys an instance of `Object` by freeing its resources.
 */
void object_destroy(struct Object *const object);