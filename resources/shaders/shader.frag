#version 450

layout(binding = 1) uniform sampler2D textureSampler2D;

layout(location = 0) in vec3 fragmentNormal;
layout(location = 1) in vec3 fragmentTextureCoordinates;
layout(location = 2) in vec4 fragmentColor;

layout(location = 0) out vec4 outColor;

void main() {
  // Take out the `z` component of the texture coordinates since we are using a 2D texture sampler
  const vec4 textureColor = texture(textureSampler2D, fragmentTextureCoordinates.xy);
  outColor = mix(textureColor, fragmentColor, fragmentColor.a);
}