#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
} uniformBufferObject;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoordinates;
layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragmentTexureCoordinates;

void main() {
  fragmentColor = inColor;
  fragmentTexureCoordinates = inTextureCoordinates;
  gl_Position = (
    uniformBufferObject.projection *
    uniformBufferObject.view *
    uniformBufferObject.model *
    vec4(inPosition, 1.0)
  );
}