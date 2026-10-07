#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	//! \todo Implement this function
	glm::mat2 const M(
		 1.0f, -1.0f,
		 0.0f,  1.0f
	);

	glm::vec2 const weights =
		glm::vec2(1.0f, x) * M;

	return weights.x * p0 + weights.y * p1;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	//! \todo Implement this function
	glm::mat4 const M(
		 0.0f, -t,          2.0f * t,       -t,
		 1.0f,  0.0f,       t - 3.0f,        2.0f - t,
		 0.0f,  t,          3.0f - 2.0f*t,   t - 2.0f,
		 0.0f,  0.0f,      -t,               t
	);

	glm::vec4 const weights =
		glm::vec4(1.0f, x, x * x, x * x * x) * M;

	return weights.x * p0
		 + weights.y * p1
		 + weights.z * p2
		 + weights.w * p3;
}
