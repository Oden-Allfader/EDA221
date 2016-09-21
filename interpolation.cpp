#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	//! \todo Implement this function
	glm::vec3 px = p0*(1.0f - x) + x*p1;
	return px;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
	glm::vec3 const& p2, glm::vec3 const& p3,
	float const t, float const x)
{
	//! \todo Implement this function
	glm::vec3 px = p0*((-t)*x + 2.0f*t*pow(x, 2.0f) - t*pow(x, 3.0f))
		+ p1*(1.0f + pow(x, 2.0f)*(t - 3) + pow(x, 3.0f)*(2 - t))
		+ p2*(t*x + pow(x, 2.0f)*(3 - 2 * t) + pow(x, 3.0f)*(t - 2))
		+ p3*(-t*pow(x, 2.0f) + t*pow(x, 3));
	return px;
}
