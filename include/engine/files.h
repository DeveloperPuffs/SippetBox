#pragma once

/**
 * Stores data about a bitmap image, captured at a specific time.
 */
typedef struct Image
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
} Image;

/**
 * Creates an instance of `Image` and initializes its resources.
 */

Image *image_create(const char *const path);

/**
 * Destroys an instance of `Image` by freeing its resources.
 */
void image_destroy(Image *const image);

/**
 * Stores data about a SPIR-V shader file.
 */
typedef struct Shader
{
    /**
     * The size of the shader code in bytes.
     */
    unsigned long size;

    /**
     * The binary data of the shader code.
     */
    unsigned int *data;
} Shader;

/**
 * Creates an instance of `Shader` and initializes its resources.
 */
Shader *shader_create(const char *const path);

/**
 * Destroys an instance of `Shader` by freeing its resources.
 */
void shader_destroy(Shader *const shader);