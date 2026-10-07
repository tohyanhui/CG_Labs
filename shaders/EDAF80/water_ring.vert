#version 410

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float elapsed_time_s;

out VS_OUT {
    vec3 vertex;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec2 normalCoord0;
    vec2 normalCoord1;
    vec2 normalCoord2;
} vs_out;

vec3 wave_position(float theta, float phi)
{
    float t = elapsed_time_s;

    float height =
        0.006 * sin(3.0 * phi + 2.0 * theta - 1.8 * t)
      + 0.003 * sin(5.0 * phi - 2.0 * theta + 1.3 * t);

    float tube_radius = 0.2 + height;

    return vec3(
        (2.0 + tube_radius * cos(theta)) * cos(phi),
        -tube_radius * sin(theta),
        (2.0 + tube_radius * cos(theta)) * sin(phi)
    );
}

void main()
{
    vec2 radial = normalize(vertex.xz);
    vec3 original_normal = normalize(normal);

    float theta = atan(
        -original_normal.y,
        dot(original_normal.xz, radial)
    );
    float phi = atan(vertex.z, vertex.x);

    vec3 displaced_vertex = wave_position(theta, phi);

    // Calculate the surface directions after deformation.
    float epsilon = 0.001;

    vec3 T = normalize(
        wave_position(theta + epsilon, phi)
      - wave_position(theta - epsilon, phi)
    );

    vec3 B = normalize(
        wave_position(theta, phi + epsilon)
      - wave_position(theta, phi - epsilon)
    );

    vec3 N = normalize(cross(B, T));

    vec4 world_position =
        vertex_model_to_world * vec4(displaced_vertex, 1.0);

    vs_out.vertex = world_position.xyz;

    vs_out.normal = normalize(
        vec3(normal_model_to_world * vec4(N, 0.0))
    );

    vs_out.tangent = normalize(
        vec3(vertex_model_to_world * vec4(T, 0.0))
    );

    vs_out.binormal = normalize(
        vec3(vertex_model_to_world * vec4(B, 0.0))
    );

    vec2 uv = texcoord.xy * vec2(2.0, 8.0);
    vec2 scroll = elapsed_time_s * vec2(-0.04, 0.02);

    vs_out.normalCoord0 = uv + scroll;
    vs_out.normalCoord1 = uv * 2.0 + scroll * 2.0;
    vs_out.normalCoord2 = uv * 4.0 + scroll * 3.0;

    gl_Position = vertex_world_to_clip * world_position;
}