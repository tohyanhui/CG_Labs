#version 410

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

out vec3 cubemap_direction;

void main()
{
    vec4 world_position = vertex_model_to_world * vec4(vertex, 1.0);

    cubemap_direction = vertex;

    gl_Position = vertex_world_to_clip * world_position;
}