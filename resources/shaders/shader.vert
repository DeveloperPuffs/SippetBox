#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
} uniformBufferObject;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTextureCoordinates;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec3 fragmentNormal;
layout(location = 1) out vec3 fragmentTexureCoordinates;
layout(location = 2) out vec4 fragmentColor;

void main() {
  gl_Position = (
    uniformBufferObject.projection *
    uniformBufferObject.view *
    uniformBufferObject.model *
    vec4(inPosition, 1.0)
  );

  // Leave these values as they are
  fragmentNormal = inNormal;
  fragmentTexureCoordinates = inTextureCoordinates;
  fragmentColor = inColor;
}