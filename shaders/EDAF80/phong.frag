#version 410

in vec3 world_position;
in vec3 world_normal;
in vec3 world_tangent;
in vec3 world_binormal;
in vec2 vertex_texcoord;

uniform vec3 light_position;
uniform vec3 camera_position;

uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform float shininess_value;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D normal_texture;

uniform int has_diffuse_texture;
uniform int has_specular_texture;
uniform int has_normal_texture;
uniform int use_normal_mapping;

out vec4 frag_color;

void main()
{
    vec3 N = normalize(world_normal);

    if (use_normal_mapping != 0 && has_normal_texture != 0)
    {
        vec3 normal_from_texture =
            texture(normal_texture, vertex_texcoord).rgb;

        normal_from_texture = normal_from_texture * 2.0 - 1.0;

        mat3 TBN = mat3(
            normalize(world_tangent),
            normalize(world_binormal),
            N
        );

        N = normalize(TBN * normal_from_texture);
    }

    vec3 L = normalize(light_position - world_position);
    vec3 V = normalize(camera_position - world_position);

    vec3 ambient_component = ambient_colour;

    vec3 kd = diffuse_colour;
    if (has_diffuse_texture != 0)
        kd = texture(diffuse_texture, vertex_texcoord).rgb;

    float diffuse_factor = max(dot(N, L), 0.0);
    vec3 diffuse_component = kd * diffuse_factor;

    vec3 ks = specular_colour;
    if (has_specular_texture != 0)
        ks = texture(specular_texture, vertex_texcoord).rgb;

    vec3 R = reflect(-L, N);
    float specular_factor = pow(max(dot(R, V), 0.0), shininess_value);
    vec3 specular_component = ks * specular_factor;

    frag_color = vec4(
        ambient_component + diffuse_component + specular_component,
        1.0
    );
}