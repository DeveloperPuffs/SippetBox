#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include <engine/logging.h>

#include <engine/files.h>

static unsigned char image_create_png(FILE *file, Image *image);

Image *image_create(const char *const path)
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
        log_error("Failed to allocate memory for extension");
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

    Image *image = calloc(1, sizeof(Image));
    if (image == NULL)
    {
        log_error("Failed to allocate memory for image instance");
        free(extension);
        fclose(file);
        return NULL;
    }

    unsigned char (*image_creation_function)(FILE *file, Image *image);
    if (strcmp(extension, ".png") == 0)
    {
        image_creation_function = image_create_png;
    }
    else
    {
        log_error("Unsupported image extension '%s'", extension);
        image_destroy(image);
        free(extension);
        fclose(file);
        return NULL;
    }

    free(extension);
    const unsigned char image_creation_result = image_creation_function(file, image);
    fclose(file);

    if (image_creation_result == 0)
    {
        log_error("Failed to create image with: %s", path);
        image_destroy(image);
        return NULL;
    }

    return image;
}

static unsigned char image_create_png(FILE *file, Image *image)
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

void image_destroy(Image *const image)
{
    for (unsigned int row = 0; row < image->height; ++row)
    {
        free(image->data[row]);
    }

    free(image->data);
    free(image);
}