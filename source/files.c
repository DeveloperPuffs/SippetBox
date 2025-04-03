#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include <sippet/core/logging.h>
#include <sippet/core/math.h>
#include <sippet/core/collections.h>
#include <sippet/files.h>

static inline void closeFile(const char *const type, FILE *file) {
  const int result = fclose(file);
  if (result) sippetLogWarning("Failed to close %s file, error code: %d", type, result);
}

static bool sippetCreatePNGImage(FILE *file, struct SippetImage *const image);

struct SippetImage *sippetCreateImage(const char *const path) {
  const char *dotPosition = strrchr(path, '.');
  if (!dotPosition) return (sippetLogError("Failed to find image extension for: %s", path), NULL);

  const size_t extensionLength = strlen(dotPosition) + 1;
  char *const extension = (char *)malloc(extensionLength * sizeof(char));
  if (!extension) return NULL;

  strcpy(extension, dotPosition);

  FILE *file = fopen(path, "rb");
  if (!file) return (sippetLogError("Failed to open image: %s", path), free(extension), NULL);

  struct SippetImage *const image = calloc(1, sizeof(struct SippetImage));
  if (!image) return (closeFile("image", file), free(extension), NULL);

  bool createdImage;
  if (!strcmp(extension, ".png")) createdImage = sippetCreatePNGImage(file, image);
  else return (sippetLogError("Unsupported image extension '%s'", extension), closeFile("image", file), sippetDestroyImage(image), free(extension), NULL);

  free(extension);
  closeFile("image", file);
  return createdImage ? image : (sippetLogError("Failed to create image with: %s", path), sippetDestroyImage(image), NULL);
}

static bool sippetCreatePNGImage(FILE *file, struct SippetImage *const image) {
  png_structp reader = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!reader) return (sippetLogError("Failed to create PNG reader struct"), false);

  png_infop information = png_create_info_struct(reader);
  if (!information) return (sippetLogError("Failed to query PNG reader struct information"), png_destroy_read_struct(&reader, &information, NULL), false);
  if (setjmp(png_jmpbuf(reader))) return (sippetLogError("Failed to process PNG"), png_destroy_read_struct(&reader, &information, NULL), false);

  png_init_io(reader, file);
  png_read_info(reader, information);
  image->width = (unsigned int)png_get_image_width(reader, information);
  image->height = (unsigned int)png_get_image_height(reader, information);
  image->depth = png_get_bit_depth(reader, information);
  if (image->depth == 16) png_set_strip_16(reader);

  const png_byte colorType = png_get_color_type(reader, information);
  if (colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(reader);
  if (colorType == PNG_COLOR_TYPE_GRAY && image->depth < 8) png_set_expand_gray_1_2_4_to_8(reader);
  if (png_get_valid(reader, information, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(reader);
  if (colorType != PNG_COLOR_TYPE_RGBA) png_set_filler(reader, 0xFF, PNG_FILLER_AFTER);

  png_read_update_info(reader, information);
  if (!(image->data = calloc(image->height, sizeof(unsigned char *))))
    return (png_destroy_read_struct(&reader, &information, NULL), false);

  for (unsigned int i = 0; i < image->height; ++i) {
    if (!(image->data[i] = malloc(png_get_rowbytes(reader, information))))
      return (png_destroy_read_struct(&reader, &information, NULL), false);
  }

  png_read_image(reader, (png_bytepp)image->data);
  png_destroy_read_struct(&reader, &information, NULL);
  return true;
}

void sippetDestroyImage(struct SippetImage *const image) {
  if (!image) return;
  for (unsigned int i = 0; i < image->height; ++i) free(image->data[i]);
  free(image->data);
  free(image);
}

struct SippetShader *sippetCreateShader(const char *const path) {
  FILE *file = fopen(path, "rb");
  if (!file) return (sippetLogError("Failed to open shader: %s", path), NULL);

  struct SippetShader *const shader = calloc(1, sizeof(struct SippetShader));
  if (!shader) return (closeFile("shader", file), NULL);

  if (fseek(file, 0, SEEK_END)) return (sippetLogError("Failed to seek to end of shader: %s", path), closeFile("shader", file), sippetDestroyShader(shader), NULL);

  const long size = ftell(file);
  if (size == -1L) return (sippetLogError("Failed to determine shader size: %s", path), closeFile("shader", file), sippetDestroyShader(shader), NULL);
  if (size % 4) return (sippetLogError("Shader size is not aligned: %s", path), closeFile("shader", file), sippetDestroyShader(shader), NULL);
  if (fseek(file, 0, SEEK_SET)) return (sippetLogError("Failed to rewind shader: %s", path), closeFile("shader", file), sippetDestroyShader(shader), NULL);
  if (!(shader->data = malloc(shader->size = (unsigned long)size))) return (closeFile("shader", file), sippetDestroyShader(shader), NULL);

  if (fread(shader->data, 1, size, file) != size)
    return (sippetLogError("Failed to read shader: %s", path), closeFile("shader", file), sippetDestroyShader(shader), NULL);

  closeFile("shader", file);
  return shader;
}

void sippetDestroyShader(struct SippetShader *const shader) {
  if (!shader) return;
  free(shader->data);
  free(shader);
}

struct SippetObject *sippetCreateObject(const char *const path) {
  FILE *file = fopen(path, "rb");
  if (!file) return (sippetLogError("Failed to open object: %s", path), NULL);

  struct SippetObject *const object = calloc(1, sizeof(struct SippetObject));
  if (!object) return (closeFile("object", file), NULL);

  if (!(object->positions = sippetCreateArray(64, sizeof(float))))
    return (sippetLogError("Failed to create positions array"), sippetDestroyObject(object), closeFile("object", file), NULL);

  if (!(object->normals = sippetCreateArray(64, sizeof(float))))
    return (sippetLogError("Failed to create normals array"), sippetDestroyObject(object), closeFile("object", file), NULL);

  if (!(object->textureCoordinates = sippetCreateArray(64, sizeof(float))))
    return (sippetLogError("Failed to create texture coordinates array"), sippetDestroyObject(object), closeFile("object", file), NULL);

  if (!(object->indices = sippetCreateArray(64, sizeof(uint32_t))))
    return (sippetLogError("Failed to create indices array"), sippetDestroyObject(object), closeFile("object", file), NULL);

  char line[256];
  size_t lineNumber = 0;
  while (fgets(line, sizeof(line), file)) {
    lineNumber++;

    if (!strncmp(line, "v ", 2)) {
      float position[3] = {0.0f, 0.0f, 0.0f};
      if (sscanf(line, "v %f %f %f", &position[0], &position[1], &position[2]) != 3)
        return (sippetLogError("Failed to parse position (v) at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

      if (!sippetPushArray(position, 3, object->positions))
        return (sippetLogError("Failed to push position at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

    } else if (!strncmp(line, "f ", 2)) {
      uint32_t p[3] = {1, 1, 1}, t[3] = {1, 1, 1}, n[3] = {1, 1, 1};
      const int XX = 3 == sscanf(line, "f %u %u %u",                   &p[0], &p[1], &p[2]);
      const int TX = 6 == sscanf(line, "f %u/%u %u/%u %u/%u",          &p[0], &t[0], &p[1], &t[1], &p[2], &t[2]);
      const int XN = 6 == sscanf(line, "f %u//%u %u//%u %u//%u",       &p[0], &n[0], &p[1], &n[1], &p[2], &n[2]);
      const int TN = 9 == sscanf(line, "f %u/%u/%u %u/%u/%u %u/%u/%u", &p[0], &t[0], &n[0], &p[1], &t[1], &n[1], &p[2], &t[2], &n[2]);
      if (!(XX + TX + XN + TN))
        return (sippetLogError("Failed to parse face (f) at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

      if (!(XN + TN)) {
        // If the normal is not provided, calculate it based on the positions
        const float positions[9] = {
          ((float *)object->positions->data)[(p[0] - 1) * 3 + 0],
          ((float *)object->positions->data)[(p[0] - 1) * 3 + 1],
          ((float *)object->positions->data)[(p[0] - 1) * 3 + 2],
          ((float *)object->positions->data)[(p[1] - 1) * 3 + 0],
          ((float *)object->positions->data)[(p[1] - 1) * 3 + 1],
          ((float *)object->positions->data)[(p[1] - 1) * 3 + 2],
          ((float *)object->positions->data)[(p[2] - 1) * 3 + 0],
          ((float *)object->positions->data)[(p[2] - 1) * 3 + 1],
          ((float *)object->positions->data)[(p[2] - 1) * 3 + 2]
        };

        const float vector1[3] = {positions[3] - positions[0], positions[4] - positions[1], positions[5] - positions[2]};
        const float vector2[3] = {positions[6] - positions[0], positions[7] - positions[1], positions[8] - positions[2]};

        float normal[3];
        sippetCross3D(vector1, vector2, normal);
        sippetNormalize3D(normal);

        if (!sippetPushArray(normal, 3, object->normals))
          return (sippetLogError("Failed to push computed normal for face at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

        n[0] = n[1] = n[2] = object->normals->length / 3;
      }

      const uint32_t indices[9] = {
        --p[0], --t[0], --n[0],
        --p[1], --t[1], --n[1],
        --p[2], --t[2], --n[2]
      };

      if (!sippetPushArray(indices, 9, object->indices))
        return (sippetLogError("Failed to push indices of face at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

    } else if (!strncmp(line, "vn ", 3)) {
      float normal[3] = {0.0f, 0.0f, 0.0f};
      if (sscanf(line, "vn %f %f %f", &normal[0], &normal[1], &normal[2]) != 3)
        return (sippetLogError("Failed to parse normal (vn) at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

      if (!sippetPushArray(normal, 3, object->normals))
        return (sippetLogError("Failed to push normal at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

    } else if (!strncmp(line, "vt ", 3)) {
      float textureCoordinates[3] = {0.0f, 0.0f, 1.0f};
      const int parsedValues = sscanf(line, "vt %f %f %f", &textureCoordinates[0], &textureCoordinates[1], &textureCoordinates[2]);
      if (parsedValues != 2 && parsedValues != 3)
        return (sippetLogError("Failed to parse texture coordinates (vt) at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

      if (!sippetPushArray(textureCoordinates, 3, object->textureCoordinates))
        return (sippetLogError("Failed to push normal at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

    } else if (!strncmp(line, "g ", 2)) {
      char groupName[128];
      if (sscanf(line, "g %s", groupName) != 1)
        return (sippetLogError("Failed to parse group (g) at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

    } else if (!strncmp(line, "o ", 2)) {
      char objectName[128];
      if (sscanf(line, "o %s", objectName) != 1)
        return (sippetLogError("Failed to parse object (o) at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

    } else if (!strncmp(line, "mtllib ", 7)) {
      char materialFile[128];
      if (sscanf(line, "mtllib %s", materialFile) != 1)
        return (sippetLogError("Failed to parse material file (mtllib) at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);

    } else if (!strncmp(line, "usemtl ", 7)) {
      char materialName[128];
      if (sscanf(line, "usemtl %s", materialName) != 1)
        return (sippetLogError("Failed to parse material (usemtl) at line %d", lineNumber), sippetDestroyObject(object), closeFile("object", file), NULL);
    }
  }

  closeFile("object", file);
  return object;
}

void sippetDestroyObject(struct SippetObject *const object) {
  if (!object) return;
  sippetDestroyArray(object->positions);
  sippetDestroyArray(object->normals);
  sippetDestroyArray(object->textureCoordinates);
  sippetDestroyArray(object->indices);
  free(object);
}