#version 450

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} uniform_buffer_object;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 fragment_color;

void main()
{
    fragment_color = in_color;
    gl_Position =
    (
        uniform_buffer_object.projection *
        uniform_buffer_object.view *
        uniform_buffer_object.model *
        vec4(in_position, 0.0, 1.0)
    );
}