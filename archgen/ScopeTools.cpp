#include "ScopeTools.h"

#include <SM_Matrix.h>
#include <SM_Rect.h>

namespace archgen
{

bool ScopeTools::CalcFaceMapping(const pm3::Polytope& poly, const pm3::Polytope::Face& face, 
	                             sm::vec3& o, sm::vec3& x, sm::vec3& y)
{

	sm::vec3 normal;
	if (!poly.CalcFaceNormal(face, normal)) {
		return false;
	}

	auto rot = sm::mat4(sm::Quaternion::CreateFromVectors(normal, sm::vec3(0, 0, 1)));

	auto& pts = poly.Points();

	std::vector<sm::vec3> rot_face;
	rot_face.reserve(face.border.size());
	for (auto idx : face.border) {
		rot_face.push_back(rot * pts[idx]->pos);
	}

	float z = 0.0f;
	for (auto& p : rot_face) {
		z += p.z;
	}
	z /= rot_face.size();

	sm::rect r;
	r.MakeEmpty();
	for (auto& p : rot_face) {
		r.Combine(sm::vec2(p.x, p.y));
	}

	auto inv_rot = rot.Inverted();

	sm::vec3 ori = inv_rot * sm::vec3(r.xmin, r.ymin, z);
	sm::vec3 dx  = inv_rot * sm::vec3(r.xmax, r.ymin, z) - ori;
	sm::vec3 dy  = inv_rot * sm::vec3(r.xmin, r.ymax, z) - ori;

	const sm::vec3 EXPECT_Y(0, 1, 0);
	if (fabs(EXPECT_Y.Dot(dx)) - fabs(EXPECT_Y.Dot(dy)) > SM_LARGE_EPSILON) {
		std::swap(dx, dy);
	}

	o = ori;
	x = dx;
	y = dy;

	return true;
}

sm::mat4 ScopeTools::CalcInsertMat(const sm::cube& aabb, const sm::mat4& scope)
{
	auto size = aabb.Size();
	const float sx = 1.0f / size.x;
	const float sy = 1.0f / size.y;
	const float sz = 1.0f / size.z;

	auto mat_s = sm::mat4::Scaled(sx, sy, sz);

	auto c = aabb.Center();
	auto mat_t = sm::mat4::Translated(-c.x, -c.y, -c.z);

	auto mat_o = sm::mat4::Translated(0.5f, 0.5f, 0.0f);

	return scope * mat_o * mat_s * mat_t;
}

}