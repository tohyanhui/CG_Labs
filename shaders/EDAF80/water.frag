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
	vec3 N = normalize(fs_in.normal);
	vec3 n0 = texture(normal_texture, fs_in.normalCoord0).rgb * 2.0 - 1.0;
	vec3 n1 = texture(normal_texture, fs_in.normalCoord1).rgb * 2.0 - 1.0;
	vec3 n2 = texture(normal_texture, fs_in.normalCoord2).rgb * 2.0 - 1.0;

	vec3 n_bump = normalize(n0 + n1 + n2);
	mat3 TBN = mat3(
		normalize(fs_in.tangent),
		normalize(fs_in.binormal),
		normalize(fs_in.normal)
	);

	N = normalize(TBN * n_bump);
	vec3 V = normalize(camera_position - fs_in.vertex);

	float eta;

	if (!gl_FrontFacing) {
		eta = 1.0 / 1.33;
	} else {
		N = -N;
		eta = 1.33;
	}

	float facing = 1.0 - max(dot(V, N), 0.0);

	vec3 deep_colour = vec3(0.0, 0.0, 0.1);
	vec3 shallow_colour = vec3(0.0, 0.5, 0.5);

	vec3 water_colour = mix(
		deep_colour,
		shallow_colour,
		facing
	);

	vec3 R = reflect(-V, N);
	vec3 reflection = texture(cubemap, R).rgb;
	float fresnel =
		0.02037
		+ (1.0 - 0.02037)
		* pow(1.0 - dot(V, N), 5.0);
	vec3 T = refract(-V, N, eta);
	vec3 refraction = texture(cubemap, T).rgb;

	frag_color = vec4(
		water_colour
		+ reflection * fresnel
		+ refraction * (1.0 - fresnel),
		1.0
	);

	// frag_color = vec4(refraction, 1.0);
}