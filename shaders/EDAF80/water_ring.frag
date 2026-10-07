#version 410

uniform vec3 camera_position;
uniform samplerCube cubemap;
uniform sampler2D normal_texture;

in VS_OUT {
    vec3 vertex;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec2 normalCoord0;
    vec2 normalCoord1;
    vec2 normalCoord2;
} fs_in;

out vec4 frag_color;

void main()
{
    vec3 n0 = texture(normal_texture, fs_in.normalCoord0).rgb * 2.0 - 1.0;
    vec3 n1 = texture(normal_texture, fs_in.normalCoord1).rgb * 2.0 - 1.0;
    vec3 n2 = texture(normal_texture, fs_in.normalCoord2).rgb * 2.0 - 1.0;

    vec3 bump = normalize(n0 + n1 + n2);
    bump = normalize(vec3(bump.xy * 0.10, bump.z));

    vec3 baseN = normalize(fs_in.normal);
    vec3 tangent = normalize(
        fs_in.tangent - baseN * dot(fs_in.tangent, baseN)
    );
    vec3 binormal = normalize(cross(tangent, baseN));

    mat3 TBN = mat3(tangent, binormal, baseN);
    vec3 N = normalize(TBN * bump);
    vec3 V = normalize(camera_position - fs_in.vertex);

    bool outside = dot(V, baseN) >= 0.0;
    if (dot(V, N) < 0.0)
        N = -N;

    float eta = outside ? 1.0 / 1.33 : 1.33;
    float cos_view = clamp(dot(V, N), 0.0, 1.0);

    float fresnel = 0.02037
        + (1.0 - 0.02037) * pow(1.0 - cos_view, 5.0);

    vec3 reflection = texture(cubemap, reflect(-V, N)).rgb;
    vec3 direction = refract(-V, N, eta);
    vec3 refraction = reflection;

    if (dot(direction, direction) > 0.00001)
        refraction = texture(cubemap, direction).rgb;
    else
        fresnel = 1.0;

    vec3 tint = vec3(0.90, 0.97, 1.0);
    vec3 colour = mix(refraction * tint, reflection, fresnel);

    frag_color = vec4(colour, 1.0);
}