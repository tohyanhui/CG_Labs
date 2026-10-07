#version 410

// Remember how we enabled vertex attributes in assignment 2 and attached some
// data to each of them, here we retrieve that data. Attribute 0 pointed to the
// vertices inside the OpenGL buffer object, so if we say that our input
// variable `vertex` is at location 0, which corresponds to attribute 0 of our
// vertex array, vertex will be effectively filled with vertices from our
// buffer.
// Similarly, normal is set to location 1, which corresponds to attribute 1 of
// the vertex array, and therefore will be filled with normals taken out of our
// buffer.
layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float elapsed_time_s;

// This is the custom output of this shader. If you want to retrieve this data
// from another shader further down the pipeline, you need to declare the exact
// same structure as in (for input), with matching name for the structure
// members and matching structure type. Have a look at
// shaders/EDAF80/diffuse.frag.
out VS_OUT {
    vec3 vertex;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec2 normalCoord0;
    vec2 normalCoord1;
    vec2 normalCoord2;
} vs_out;

void wave(
    vec2 position,
    float amplitude,
    vec2 direction,
    float frequency,
    float phase,
    float sharpness,
    out float height,
    out float derivative_x,
    out float derivative_z)
{
    float angle =
        dot(direction, position) * frequency
        + elapsed_time_s * phase;

    float alpha =
        sin(angle) * 0.5 + 0.5;

    height =
        amplitude * pow(alpha, sharpness);

    derivative_x =
        0.5 * sharpness * frequency * amplitude
        * pow(alpha, sharpness - 1.0)
        * cos(angle)
        * direction.x;

    derivative_z =
        0.5 * sharpness * frequency * amplitude
        * pow(alpha, sharpness - 1.0)
        * cos(angle)
        * direction.y;
}

void main()
{
    float height1;
    float derivative_x1;
    float derivative_z1;

    float height2;
    float derivative_x2;
    float derivative_z2;

    wave(
        vertex.xz,
        1.0,
        vec2(-1.0, 0.0),
        0.2,
        0.5,
        2.0,
        height1,
        derivative_x1,
        derivative_z1
    );

    wave(
        vertex.xz,
        0.5,
        vec2(-0.7, 0.7),
        0.4,
        1.3,
        2.0,
        height2,
        derivative_x2,
        derivative_z2
    );

    vec3 displaced_vertex = vertex;
    displaced_vertex.y = height1 + height2;

	float derivative_x = derivative_x1 + derivative_x2;
	float derivative_z = derivative_z1 + derivative_z2;

	vec3 tangent_x = vec3(1.0, derivative_x, 0.0);
	vec3 tangent_z = vec3(0.0, derivative_z, 1.0);

	vs_out.tangent = normalize(
		vec3(vertex_model_to_world * vec4(tangent_x, 0.0))
	);

	vs_out.binormal = normalize(
		vec3(vertex_model_to_world * vec4(tangent_z, 0.0))
	);

	vec3 normal = normalize(cross(tangent_z, tangent_x));

	vs_out.normal = normalize(
		vec3(normal_model_to_world * vec4(normal, 0.0))
	);

    vs_out.vertex =
        vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));

	vec2 texScale = vec2(8.0, 4.0);
	float normalTime = mod(elapsed_time_s, 100.0);
	vec2 normalSpeed = vec2(-0.05, 0.0);

	vs_out.normalCoord0 =
		texcoord.xy * texScale
		+ normalTime * normalSpeed;

	vs_out.normalCoord1 =
		texcoord.xy * texScale * 2.0
		+ normalTime * normalSpeed * 4.0;

	vs_out.normalCoord2 =
		texcoord.xy * texScale * 4.0
		+ normalTime * normalSpeed * 8.0;

	gl_Position =
        vertex_world_to_clip
        * vertex_model_to_world
        * vec4(displaced_vertex, 1.0);
}
