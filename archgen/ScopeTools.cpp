#include "ScopeTools.h"

#include <SM_Matrix.h>
#include <SM_Rect.h>
#include <SM_Calc.h>
#include <halfedge/Utility.h>

namespace
{

class FaceToNorm
{
public:
	sm::vec3 calc_normal(const he::loop3* face)
	{
		auto itr = face2norm.find(face);
		if (itr != face2norm.end()) 
		{
			return itr->second;
		} 
		else 
		{
			sm::Plane plane;
			he::Utility::LoopToPlane(*face, plane);
			auto& n = plane.normal;

			face2norm.insert({ face, n });
			return n;
		}
	}

private:
	std::map<const he::loop3*, sm::vec3> face2norm;

}; // FaceToNorm

}

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

std::vector<sm::mat4> ScopeTools::CalcEdgesMapping(const pm3::Polytope& poly)
{
	std::vector<sm::mat4> mats;

	auto topo = const_cast<pm3::Polytope&>(poly).GetTopoPoly();

	FaceToNorm f2n;

	std::set<he::edge3*> checked_edges;

	auto first_edge = topo->GetEdges().Head();
	auto curr_edge = first_edge;
	do {
		if (!curr_edge->twin ||
			checked_edges.find(curr_edge) != checked_edges.end() ||
			checked_edges.find(curr_edge->twin) != checked_edges.end()) {
			curr_edge = curr_edge->linked_next;
			continue;
		}

		checked_edges.insert(curr_edge);
		checked_edges.insert(curr_edge->twin);

		auto& p0 = curr_edge->vert->position;
		auto& p1 = curr_edge->next->vert->position;

		sm::mat4 mat;

		mat.Scale(sm::dis_pos3_to_pos3(p0, p1), 1.0f, 1.0f);

		sm::vec3 norm = (f2n.calc_normal(curr_edge->loop) + f2n.calc_normal(curr_edge->twin->loop)).Normalized();
		sm::vec3 dir_x = (p1 - p0).Normalized();
		sm::vec3 dir_y = norm;
		sm::vec3 dir_z = dir_x.Cross(dir_y);
		mat = sm::mat4(sm::mat3(dir_x, dir_y, dir_z)) * mat;

		mat.Translate(p0.x, p0.y, p0.z);

		mats.push_back(mat);

		curr_edge = curr_edge->linked_next;
	} while (curr_edge != first_edge);

	return mats;
}

std::vector<sm::mat4> ScopeTools::CalcFaceEdgesMapping(const pm3::Polytope& poly)
{
	std::vector<sm::mat4> mats;

	auto topo = const_cast<pm3::Polytope&>(poly).GetTopoPoly();

	FaceToNorm f2n;

	auto first_edge = topo->GetEdges().Head();
	auto curr_edge = first_edge;
	do {
		auto& p0 = curr_edge->vert->position;
		auto& p1 = curr_edge->next->vert->position;

		sm::mat4 mat;

		mat.Scale(sm::dis_pos3_to_pos3(p0, p1), 1.0f, 1.0f);

		sm::vec3 dir_x = (p1 - p0).Normalized();
		sm::vec3 dir_y = f2n.calc_normal(curr_edge->loop);
		sm::vec3 dir_z = dir_x.Cross(dir_y);
		mat = sm::mat4(sm::mat3(dir_x, dir_y, dir_z)) * mat;

		mat.Translate(p0.x, p0.y, p0.z);

		mats.push_back(mat);

		curr_edge = curr_edge->linked_next;
	} while (curr_edge != first_edge);

	return mats;
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