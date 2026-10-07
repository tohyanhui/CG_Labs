#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

out vec3 world_position;
out vec3 world_normal;
out vec3 world_tangent;
out vec3 world_binormal;
out vec2 vertex_texcoord;

void main()
{
    vec4 position = vertex_model_to_world * vec4(vertex, 1.0);

    world_position = position.xyz;
    world_normal = normalize((normal_model_to_world * vec4(normal, 0.0)).xyz);
    world_tangent = normalize((normal_model_to_world * vec4(tangent, 0.0)).xyz);
    world_binormal = normalize((normal_model_to_world * vec4(binormal, 0.0)).xyz);
    vertex_texcoord = texcoord.xy;

    gl_Position = vertex_world_to_clip * position;
}