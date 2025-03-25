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
    char *const extension = (char *)malloc(extension_length * sizeof(char));
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

    struct Image *const image = calloc(1, sizeof(struct Image));
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
    if (image_creation_function(file, image) == 0)
    {
        log_error("Failed to create image with: %s", path);
        file_close("image", file);
        image_destroy(image);
        return NULL;
    }

    file_close("image", file);
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

    struct Shader *const shader = calloc(1, sizeof(struct Shader));
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

static unsigned char array_push
(
    const void *const push_elements, const unsigned long push_count, const unsigned long element_size,
    unsigned long *const array_count, unsigned long *const array_capacity, void ***const array
)
{
    if (*array_count + push_count > *array_capacity)
    {
        *array_capacity = (*array_capacity == 0) ? push_count : *array_capacity * 2;

        void *reallocated_array = realloc(*array, *array_capacity * element_size);
        if (reallocated_array == NULL)
        {
            log_error("Failed to allocate memory for resizing array");
            return 0;
        }

        *array = reallocated_array;
    }

    memcpy((char *)(*array) + (*array_count) * element_size, push_elements, push_count * element_size);
    *array_count += push_count;

    return 1;
}

struct Object *object_create(const char *const path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        log_error("Failed to open object: %s", path);
        return NULL;
    }

    struct Object *const object = calloc(1, sizeof(struct Object));
    if (object == NULL)
    {
        log_error("Failed to allocate memory for object instance");
        file_close("object", file);
        return NULL;
    }

    // Please don't use a `.obj` model with over 256 characters per line :(
    char line[256];

    size_t line_number = 0;
    while (fgets(line, sizeof(line), file))
    {
        line_number++;

        if (strncmp(line, "v ", 2) == 0)
        {
            float vertex[3] = {0.0f, 0.0f, 0.0f};
            if (sscanf(line, "v %f %f %f", &vertex[0], &vertex[1], &vertex[2]) != 3)
            {
                log_error("Failed to parse vertex (v) at line %d", line_number);
                object_destroy(object);
                return NULL;
            }

            if
            (
                array_push
                (
                    vertex, 3, sizeof(float), &object->vertex_count, &object->vertex_capacity,
                    (void ***)object->vertices
                ) == 0
            )
            {
                log_error("Failed to push vertex at line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "f ", 2) == 0)
        {
            unsigned int vertex_indices[3] = {1, 1, 1};
            unsigned int texture_coordinate_indices[3] = {1, 1, 1};
            unsigned int normal_indices[3] = {1, 1, 1};

            if
            (
                sscanf
                (
                    line, "f %u %u %u",
                    &vertex_indices[0],
                    &vertex_indices[1],
                    &vertex_indices[2]
                ) != 3 &&
                sscanf
                (
                    line, "f %u/%u/%u %u/%u/%u %u/%u/%u",
                    &vertex_indices[0], &texture_coordinate_indices[0], &normal_indices[0],
                    &vertex_indices[1], &texture_coordinate_indices[1], &normal_indices[1],
                    &vertex_indices[2], &texture_coordinate_indices[2], &normal_indices[2]
                ) != 9 &&
                sscanf
                (
                    line, "f %u//%u %u//%u %u//%u",
                    &vertex_indices[0], &normal_indices[0],
                    &vertex_indices[1], &normal_indices[1],
                    &vertex_indices[2], &normal_indices[2]
                ) != 3
            )
            {
                log_error("Failed to parse face (f) at line %d", line_number);
                object_destroy(object);
                return NULL;
            }

            const unsigned int indices[9] =
            {
                --vertex_indices[0], --texture_coordinate_indices[0], --normal_indices[0],
                --vertex_indices[1], --texture_coordinate_indices[1], --normal_indices[1],
                --vertex_indices[2], --texture_coordinate_indices[2], --normal_indices[2]
            };

            if
            (
                array_push
                (
                    indices, 9, sizeof(unsigned int), &object->index_count, &object->index_capacity,
                    (void ***)object->indices
                ) == 0
            )
            {
                log_error("Failed to push indices of face at line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "vn ", 3) == 0)
        {
            float normal[3] = {0.0f, 0.0f, 0.0f};
            if (sscanf(line, "vn %f %f %f", &normal[0], &normal[1], &normal[2]) == 3)
            {
                if
                (
                    array_push
                    (
                        normal, 3, sizeof(float), &object->normal_count, &object->normal_capacity,
                        (void ***)object->normals
                    ) == 0
                )
                {
                    log_error("Failed to push normal at line %d", line_number);
                    object_destroy(object);
                    return NULL;
                }
            }
            else
            {
                log_error("Failed to parse normal (vn) at line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "vt ", 3) == 0)
        {
            float texture_coordinates[3] = {0.0f, 0.0f, 1.0f};
            const int parsed_values = sscanf
            (
                line, "vt %f %f %f", &texture_coordinates[0], &texture_coordinates[1], &texture_coordinates[2]
            );

            if (parsed_values == 2 || parsed_values == 3)
            {
                if
                (
                    array_push
                    (
                        texture_coordinates, 3, sizeof(float), &object->texture_coordinate_count,
                        &object->texture_coordinate_capacity, (void ***)object->texture_coordinates
                    ) == 0
                )
                {
                    log_error("Failed to push normal at line %d", line_number);
                    object_destroy(object);
                    return NULL;
                }
            }
            else
            {
                log_error("Failed to parse texture coordinates (vt) at line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "g ", 2) == 0)
        {
            char group_name[128];
            if (sscanf(line, "g %s", group_name) == 1)
            {
                // Do something with group name
            }
            else
            {
                log_error("Failed to parse group (g) at line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "o ", 2) == 0)
        {
            char object_name[128];
            if (sscanf(line, "o %s", object_name) == 1)
            {
                // Do something with object name
            }
            else
            {
                log_error("Failed to parse object (o) at line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "mtllib ", 7) == 0)
        {
            char material_file[128];
            if (sscanf(line, "mtllib %s", material_file) == 1)
            {
                // Do something with material file
            }
            else
            {
                log_error("Failed to parse material file (mtllib) at line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "usemtl ", 7) == 0)
        {
            char material_name[128];
            if (sscanf(line, "usemtl %s", material_name) == 1)
            {
                // Do something with material name
            }
            else
            {
                log_error("Failed to parse material (usemtl) at line %d", line_number);
                object_destroy(object);
                return NULL;
            }
        }
        else if (strncmp(line, "# ", 2) == 0)
        {
            continue;
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

void object_destroy(struct Object *const object)
{
    if (object == NULL)
    {
        return;
    }

    const size_t material_file_count = object->material_file_count;
    for (size_t material_file_index = 0; material_file_index < material_file_count; ++material_file_index)
    {
        free(object->material_files[material_file_index]);
    }

    free(object->material_files);
    free(object->normals);
    free(object->vertices);
    free(object->indices);
    free(object);
}