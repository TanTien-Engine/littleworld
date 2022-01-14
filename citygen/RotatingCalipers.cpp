#include "RotatingCalipers.h"

#include <SM_Rect.h>
#include <SM_Calc.h>

namespace citygen
{

std::pair<sm::rect, float> 
RotatingCalipers::CalcOBB(const std::vector<sm::vec2>& poly, bool loop)
{
	if (poly.empty()) {
		return std::make_pair(sm::rect(), 0.0f);
	} else if (poly.size() == 1) {
		return std::make_pair(sm::rect(poly[0], 0, 0), 0.0f);
	}

	float min_area = FLT_MAX;
	sm::rect min_aabb;
	float min_angle = 0;

	const size_t n = loop ? poly.size() : poly.size() - 1;
	for (size_t i = 0; i < n; ++i)
	{
		size_t i_next = i == poly.size() - 1 ? 0 : i + 1;
		float angle = sm::get_line_angle(poly[i], poly[i_next]);

		// rot
		auto rot_poly = poly;
		for (auto& p : rot_poly) {
			p = sm::rotate_vector(p, -angle);
		}

		// aabb
		sm::rect aabb;
		for (auto& p : rot_poly) {
			aabb.Combine(p);
		}
		float area = aabb.Width() * aabb.Height();
		if (area < min_area) 
		{
			min_area = area;
			min_aabb = aabb;
			min_angle = angle;
		}
	}

	return std::make_pair(min_aabb, min_angle);
}

}