#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include <engine/logging.h>
#include <engine/files.h>

static inline void fileClose(const char *const type, FILE *file) {
  const int result = fclose(file);
  if (result) logWarning("Failed to close %s file, error code: %d", type, result);
}

static unsigned char createImagePng(FILE *file, struct Image *const image);

struct Image *createImage(const char *const path) {
  const char *dotPosition = strrchr(path, '.');
  if (!dotPosition) return (logError("Failed to find image extension for: %s", path), NULL);

  const size_t extensionLength = strlen(dotPosition) + 1;
  char *const extension = (char *)malloc(extensionLength * sizeof(char));
  if (!extension) return NULL;

  strcpy(extension, dotPosition);

  FILE *file = fopen(path, "rb");
  if (!file) return (logError("Failed to open image: %s", path), free(extension), NULL);

  struct Image *const image = calloc(1, sizeof(struct Image));
  if (!image) return (fileClose("image", file), free(extension), NULL);

  unsigned char imageCreationResult;
  if (!strcmp(extension, ".png")) imageCreationResult = createImagePng(file, image);
  else return (logError("Unsupported image extension '%s'", extension), fileClose("image", file), destroyImage(image), free(extension), NULL);

  free(extension);
  fileClose("image", file);
  return imageCreationResult ? image : (logError("Failed to create image with: %s", path), destroyImage(image), NULL);
}

static unsigned char createImagePng(FILE *file, struct Image *const image) {
  png_structp reader = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!reader) return (logError("Failed to create PNG reader struct"), 0);

  png_infop information = png_create_info_struct(reader);
  if (!information) return (logError("Failed to query PNG reader struct information"), png_destroy_read_struct(&reader, &information, NULL), 0);
  if (setjmp(png_jmpbuf(reader))) return (logError("Failed to process PNG"), png_destroy_read_struct(&reader, &information, NULL), 0);

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
    return (png_destroy_read_struct(&reader, &information, NULL), 0);

  for (unsigned int i = 0; i < image->height; ++i) {
    if (!(image->data[i] = malloc(png_get_rowbytes(reader, information))))
      return (png_destroy_read_struct(&reader, &information, NULL), 0);
  }

  png_read_image(reader, (png_bytepp)image->data);
  png_destroy_read_struct(&reader, &information, NULL);
  return 1;
}

void destroyImage(struct Image *const image) {
  if (!image) return;
  for (unsigned int i = 0; i < image->height; ++i) free(image->data[i]);
  free(image->data);
  free(image);
}

struct Shader *createShader(const char *const path) {
  FILE *file = fopen(path, "rb");
  if (!file) return (logError("Failed to open shader: %s", path), NULL);

  struct Shader *const shader = calloc(1, sizeof(struct Shader));
  if (!shader) return (fileClose("shader", file), NULL);

  if (fseek(file, 0, SEEK_END)) return (logError("Failed to seek to end of shader: %s", path), fileClose("shader", file), destroyShader(shader), NULL);

  const long size = ftell(file);
  if (size == -1L) return (logError("Failed to determine shader size: %s", path), fileClose("shader", file), destroyShader(shader), NULL);
  if (size % 4) return (logError("Shader size is not aligned: %s", path), fileClose("shader", file), destroyShader(shader), NULL);
  if (fseek(file, 0, SEEK_SET)) return (logError("Failed to rewind shader: %s", path), fileClose("shader", file), destroyShader(shader), NULL);
  if (!(shader->data = malloc(shader->size = (unsigned long)size))) return (fileClose("shader", file), destroyShader(shader), NULL);

  if (fread(shader->data, 1, size, file) != size)
    return (logError("Failed to read shader: %s", path), fileClose("shader", file), destroyShader(shader), NULL);

  fileClose("shader", file);
  return shader;
}

void destroyShader(struct Shader *const shader) {
  if (!shader) return;
  free(shader->data);
  free(shader);
}

static unsigned char arrayPush(const void *const pushElements, const unsigned long pushCount, const unsigned long elementSize, unsigned long *const arrayCount, unsigned long *const arrayCapacity, void ***const array) {
  if (*arrayCount + pushCount > *arrayCapacity) {
    *arrayCapacity = (*arrayCapacity == 0) ? pushCount : *arrayCapacity * 2;
    void *reallocatedArray = realloc(*array, *arrayCapacity * elementSize);
    if (!reallocatedArray) return 0;
    *array = reallocatedArray;
  }

  memcpy((char *)(*array) + (*arrayCount) * elementSize, pushElements, pushCount * elementSize);
  *arrayCount += pushCount;
  return 1;
}

struct Object *createObject(const char *const path) {
  FILE *file = fopen(path, "rb");
  if (!file) return (logError("Failed to open object: %s", path), NULL);

  struct Object *const object = calloc(1, sizeof(struct Object));
  if (!object) return (fileClose("object", file), NULL);

  // Please don't use a `.obj` model with over 256 characters per line :(
  char line[256];

  size_t lineNumber = 0;
  while (fgets(line, sizeof(line), file)) {
    ++lineNumber;

    if (!strncmp(line, "v ", 2)) {
      float vertex[3] = {0.0f, 0.0f, 0.0f};
      if (sscanf(line, "v %f %f %f", &vertex[0], &vertex[1], &vertex[2]) != 3)
        return (logError("Failed to parse vertex (v) at line %d", lineNumber), destroyObject(object), NULL);

      if (!arrayPush(vertex, 3, sizeof(float), &object->vertexCount, &object->vertexCapacity, (void ***)object->vertices))
        return (logError("Failed to push vertex at line %d", lineNumber), destroyObject(object), NULL);

    } else if (!strncmp(line, "f ", 2)) {
      //           v - Vertex,       t - Texcoords,    n - Normal
      unsigned int v[3] = {1, 1, 1}, t[3] = {1, 1, 1}, n[3] = {1, 1, 1};
      const int result1 = sscanf(line, "f %u %u %u", &v[0], &v[1], &v[2]);
      const int result2 = sscanf(line, "f %u//%u %u//%u %u//%u", &v[0], &n[0], &v[1], &n[1], &v[2], &n[2]);
      const int result3 = sscanf(line, "f %u/%u/%u %u/%u/%u %u/%u/%u", &v[0], &t[0], &n[0], &v[1], &t[1], &n[1], &v[2], &t[2], &n[2]);
      if (result1 != 3 && result2 != 6 && result3 != 9)
        return (logError("Failed to parse face (f) at line %d", lineNumber), destroyObject(object), NULL);

      const unsigned int indices[9] = {--v[0], --t[0], --n[0], --v[1], --t[1], --n[1], --v[2], --t[2], --n[2]};
      if (!arrayPush(indices, 9, sizeof(unsigned int), &object->indexCount, &object->indexCapacity, (void ***)object->indices))
        return (logError("Failed to push indices of face at line %d", lineNumber), destroyObject(object), NULL);

    } else if (!strncmp(line, "vn ", 3)) {
      float normal[3] = {0.0f, 0.0f, 0.0f};
      if (sscanf(line, "vn %f %f %f", &normal[0], &normal[1], &normal[2]) != 3)
        return (logError("Failed to parse normal (vn) at line %d", lineNumber), destroyObject(object), NULL);

      if (!arrayPush(normal, 3, sizeof(float), &object->normalCount, &object->normalCapacity, (void ***)object->normals))
        return (logError("Failed to push normal at line %d", lineNumber), destroyObject(object), NULL);

    } else if (!strncmp(line, "vt ", 3)) {
      float textureCoordinates[3] = {0.0f, 0.0f, 1.0f};
      const int parsedValues = sscanf(line, "vt %f %f %f", &textureCoordinates[0], &textureCoordinates[1], &textureCoordinates[2]);
      if (parsedValues != 2 && parsedValues != 3)
        return (logError("Failed to parse texture coordinates (vt) at line %d", lineNumber), destroyObject(object), NULL);

      if (!arrayPush(textureCoordinates, 3, sizeof(float), &object->textureCoordinateCount, &object->textureCoordinateCapacity, (void ***)object->textureCoordinates))
        return (logError("Failed to push normal at line %d", lineNumber), destroyObject(object), NULL);

    } else if (!strncmp(line, "g ", 2)) {
      char groupName[128];
      if (sscanf(line, "g %s", groupName) != 1)
        return (logError("Failed to parse group (g) at line %d", lineNumber), destroyObject(object), NULL);

    } else if (!strncmp(line, "o ", 2)) {
      char objectName[128];
      if (sscanf(line, "o %s", objectName) != 1)
        return (logError("Failed to parse object (o) at line %d", lineNumber), destroyObject(object), NULL);

    } else if (!strncmp(line, "mtllib ", 7)) {
      char materialFile[128];
      if (sscanf(line, "mtllib %s", materialFile) != 1)
        return (logError("Failed to parse material file (mtllib) at line %d", lineNumber), destroyObject(object), NULL);

    } else if (!strncmp(line, "usemtl ", 7)) {
      char materialName[128];
      if (sscanf(line, "usemtl %s", materialName) != 1)
        return (logError("Failed to parse material (usemtl) at line %d", lineNumber), destroyObject(object), NULL);

    } else if (!strncmp(line, "# ", 2)) continue;
    else return (logError("Failed to parse line %d", lineNumber), destroyObject(object), NULL);
  }

  fileClose("object", file);
  return object;
}

void destroyObject(struct Object *const object) {
  if (!object) return;
  for (unsigned int i = 0; i < object->materialFileCount; ++i) free(object->materialFiles[i]);
  free(object->materialFiles);
  free(object->normals);
  free(object->vertices);
  free(object->indices);
  free(object);
}