#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include <engine/logging.h>

#include <engine/files.h>

static inline void file_close(const char *const type, FILE *file)
{
    const int result = fclose(file);
    if (result != 0)
    {
        log_warning("Failed to close %s file, error code: %d", type, result);
    }
}

/*********************************************************************************************************************/
// Image Handling Functions
/*********************************************************************************************************************/

static unsigned char image_create_png(FILE *file, struct Image *const image);

struct Image *image_create(const char *const path)
{
    const char *dot_position = strrchr(path, '.');
    if (dot_position == NULL)
    {
        log_error("Failed to find image extension for: %s", path);
        return NULL;
    }

    const size_t extension_length = strlen(dot_position) + 1;
    char *extension = (char *)malloc(extension_length * sizeof(char));
    if (extension == NULL)
    {
        log_error("Failed to allocate memory for image extension string");
        return NULL;
    }

    strcpy(extension, dot_position);

    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        log_error("Failed to open image: %s", path);
        free(extension);
        return NULL;
    }

    struct Image *image = calloc(1, sizeof(struct Image));
    if (image == NULL)
    {
        log_error("Failed to allocate memory for image instance");
        file_close("image", file);
        free(extension);
        return NULL;
    }

    unsigned char (*image_creation_function)(FILE *file, struct Image *image);
    if (strcmp(extension, ".png") == 0)
    {
        image_creation_function = image_create_png;
    }
    else
    {
        log_error("Unsupported image extension '%s'", extension);
        file_close("image", file);
        image_destroy(image);
        free(extension);
        return NULL;
    }

    free(extension);
    const unsigned char image_creation_result = image_creation_function(file, image);
    file_close("image", file);

    if (image_creation_result == 0)
    {
        log_error("Failed to create image with: %s", path);
        image_destroy(image);
        return NULL;
    }

    return image;
}

static unsigned char image_create_png(FILE *file, struct Image *const image)
{
    png_structp reader = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (reader == NULL)
    {
        log_error("Failed to create PNG reader struct");
        return 0;
    }

    png_infop information = png_create_info_struct(reader);
    if (information == NULL)
    {
        log_error("Failed to query PNG reader struct information");
        png_destroy_read_struct(&reader, &information, NULL);
        return 0;
    }

    if (setjmp(png_jmpbuf(reader)) != 0)
    {
        log_error("Failed to process PNG");
        png_destroy_read_struct(&reader, &information, NULL);
        return 0;
    }

    png_init_io(reader, file);
    png_read_info(reader, information);

    image->width = (unsigned int)png_get_image_width(reader, information);
    image->height = (unsigned int)png_get_image_height(reader, information);

    image->depth = png_get_bit_depth(reader, information);
    if (image->depth == 16)
    {
        png_set_strip_16(reader);
    }

    const png_byte color_type = png_get_color_type(reader, information);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(reader);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY && image->depth < 8)
    {
        png_set_expand_gray_1_2_4_to_8(reader);
    }

    if (png_get_valid(reader, information, PNG_INFO_tRNS) != 0)
    {
        png_set_tRNS_to_alpha(reader);
    }

    if (color_type != PNG_COLOR_TYPE_RGBA)
    {
        png_set_filler(reader, 0xFF, PNG_FILLER_AFTER);
    }

    png_read_update_info(reader, information);

    image->data = (unsigned char **)calloc(image->height, sizeof(unsigned char *));
    if (image->data == NULL)
    {
        log_error("Failed to allocate memory for image rows");
        png_destroy_read_struct(&reader, &information, NULL);
        return 0;
    }

    for (unsigned int row = 0; row < image->height; ++row)
    {
        image->data[row] = malloc(png_get_rowbytes(reader, information));
        if (image->data[row] == NULL)
        {
            log_error("Failed to allocate memory for image column #%d", row);
            png_destroy_read_struct(&reader, &information, NULL);
            return 0;
        }
    }

    png_read_image(reader, (png_bytepp)image->data);
    png_destroy_read_struct(&reader, &information, NULL);

    return 1;
}

void image_destroy(struct Image *const image)
{
    if (image == NULL)
    {
        return;
    }

    for (unsigned int row = 0; row < image->height; ++row)
    {
        free(image->data[row]);
    }

    free(image->data);
    free(image);
}

/*********************************************************************************************************************/
// Shader Handling Functions
/*********************************************************************************************************************/

struct Shader *shader_create(const char *const path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        log_error("Failed to open shader: %s", path);
        return NULL;
    }

    struct Shader *shader = calloc(1, sizeof(struct Shader));
    if (shader == NULL)
    {
        log_error("Failed to allocate memory for shader instance");
        file_close("shader", file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        log_error("Failed to seek to end of shader: %s", path);
        file_close("shader", file);
        shader_destroy(shader);
        return NULL;
    }

    const long size = ftell(file);
    if (size == -1L)
    {
        log_error("Failed to determine shader size: %s", path);
        file_close("shader", file);
        shader_destroy(shader);
        return NULL;
    }

    if (size % 4 != 0)
    {
        log_error("Shader size (%d) is not aligned: %s", shader->size, path);
        file_close("shader", file);
        shader_destroy(shader);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        log_error("Failed to rewind shader: %s", path);
        file_close("shader", file);
        shader_destroy(shader);
        return NULL;
    }

    shader->size = (unsigned long)size;
    shader->data = malloc(size);
    if (shader->data == NULL)
    {
        log_error("Failed to allocate memory for shader data");
        file_close("shader", file);
        shader_destroy(shader);
        return NULL;
    }

    if (fread(shader->data, 1, size, file) != size)
    {
        log_error("Failed to read shader: %s", path);
        file_close("shader", file);
        shader_destroy(shader);
        return NULL;
    }

    file_close("shader", file);
    return shader;
}

void shader_destroy(struct Shader *const shader)
{
    if (shader == NULL)
    {
        return;
    }

    free(shader->data);
    free(shader);
}

/*********************************************************************************************************************/
// Object Handling Functions
/*********************************************************************************************************************/

static uint8_t push_vertex
(
    const float x, const float y, const float z, unsigned int *const vertex_count, float **const vertices,
    uint32_t *const vertex_capacity
);

static uint8_t push_index
(
    const unsigned int index, unsigned int *const index_count, unsigned int **const indices, uint32_t *const index_capacity
);

struct Object *object_create(const char *const path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        log_error("Failed to open object: %s", path);
        return NULL;
    }

    struct Object *object = calloc(1, sizeof(struct Object));
    if (object == NULL)
    {
        log_error("Failed to allocate memory for object instance");
        file_close("object", file);
        return NULL;
    }

    uint32_t vertex_capacity = 0;
    uint32_t index_capacity = 0;

    // Please don't use a `.obj` model with over 256 characters per line :(
    char line[256];

    uint32_t line_number = 0;
    while (fgets(line, sizeof(line), file))
    {
        line_number++;

        if (strncmp(line, "v ", 2) == 0)
        {
            float x, y, z;
            if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3)
            {
                push_vertex(x, y, z, &object->vertex_count, &object->vertices, &vertex_capacity);
            }
            else
            {
                log_error("Failed to parse 'v' line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "f ", 2) == 0)
        {
            unsigned int vertex1, vertex2, vertex3;
            if (sscanf(line, "f %u %u %u", &vertex1, &vertex2, &vertex3) == 3)
            {
                if (push_index(vertex1 - 1, &object->index_count, &object->indices, &index_capacity) == 0)
                {
                    log_error("Failed to push vertex 1 of index %d in line %d", object->vertex_count, line_number);
                    object_destroy(object);
                    return NULL;
                }

                if (push_index(vertex2 - 1, &object->index_count, &object->indices, &index_capacity) == 0)
                {
                    log_error("Failed to push vertex 2 of index %d in line %d", object->vertex_count, line_number);
                    object_destroy(object);
                    return NULL;
                }

                if (push_index(vertex3 - 1, &object->index_count, &object->indices, &index_capacity) == 0)
                {
                    log_error("Failed to push vertex 3 of index %d in line %d", object->vertex_count, line_number);
                    object_destroy(object);
                    return NULL;
                }
            }
            else
            {
                log_error("Failed to parse 'f' line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else
        {
            log_error("Failed to parse line %d", line_number);
            object_destroy(object);
            return NULL;
        }
    }

    file_close("object", file);
    return object;
}

static uint8_t push_vertex
(
    const float x, const float y, const float z, unsigned int *const vertex_count, float **const vertices,
    uint32_t *const vertex_capacity
)
{
    if (*vertex_count >= *vertex_capacity)
    {
        *vertex_capacity = *vertex_capacity == 0 ? 10 : *vertex_capacity * 2;
        float *reallocated_vertices = realloc(*vertices, *vertex_capacity * 3 * sizeof(float));
        if (reallocated_vertices == NULL)
        {
            log_error("Failed to reallocate memory for resizing vertices");
            return 0;
        }

        *vertices = reallocated_vertices;
    }

    const uint32_t vertex_index = *vertex_count * 3;
    (*vertices)[vertex_index + 0] = x;
    (*vertices)[vertex_index + 1] = y;
    (*vertices)[vertex_index + 2] = z;
    (*vertex_count)++;
    return 1;
}

static uint8_t push_index
(
    const unsigned int index, unsigned int *const index_count, unsigned int **const indices, uint32_t *const index_capacity
)
{
    if (*index_count >= *index_capacity)
    {
        *index_capacity = *index_capacity == 0 ? 10 : *index_capacity * 2;
        unsigned int *reallocated_indices = realloc(*indices, *index_capacity * sizeof(unsigned int));
        if (reallocated_indices == NULL)
        {
            log_error("Failed to allocate memory for resizing indices");
            return 0;
        }

        *indices = reallocated_indices;
    }

    (*indices)[(*index_count)++] = index;
    return 1;
}

void object_destroy(struct Object *const object)
{
    if (object == NULL)
    {
        return;
    }

    free(object->vertices);
    free(object->indices);
    free(object);
}