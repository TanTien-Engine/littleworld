#include "ScopeTools.h"
#include "PolytopeTools.h"

#include <halfedge/Utility.h>
#include <SM_Matrix.h>
#include <SM_Rect.h>
#include <SM_Calc.h>

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

	FaceNormalCache f2n;

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

		sm::vec3 norm = (f2n.GetNormal(curr_edge->loop) + f2n.GetNormal(curr_edge->twin->loop)).Normalized();
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

	FaceNormalCache f2n;

	auto first_edge = topo->GetEdges().Head();
	auto curr_edge = first_edge;
	do {
		auto& p0 = curr_edge->vert->position;
		auto& p1 = curr_edge->next->vert->position;

		sm::mat4 mat;

		mat.Scale(sm::dis_pos3_to_pos3(p0, p1), 1.0f, 1.0f);

		sm::vec3 dir_x = (p1 - p0).Normalized();
		sm::vec3 dir_y = f2n.GetNormal(curr_edge->loop);
		sm::vec3 dir_z = dir_x.Cross(dir_y);
		mat = sm::mat4(sm::mat3(dir_x, dir_y, dir_z)) * mat;

		mat.Translate(p0.x, p0.y, p0.z);

		mats.push_back(mat);

		curr_edge = curr_edge->linked_next;
	} while (curr_edge != first_edge);

	return mats;
}

void ScopeTools::CalcRoofEdgesMapping(const pm3::Polytope& roof, const pm3::Polytope& base, 
	                                  std::vector<sm::mat4>& eave, std::vector<sm::mat4>& hip,
	                                  std::vector<sm::mat4>& valley, std::vector<sm::mat4>& ridge)
{
	auto topo_base = const_cast<pm3::Polytope&>(base).GetTopoPoly();
	auto& faces = topo_base->GetLoops();
	if (faces.Size() != 1) {
		return;
	}

	sm::Plane base_plane;
	he::Utility::LoopToPlane(*faces.Head(), base_plane);

	auto topo_roof = const_cast<pm3::Polytope&>(roof).GetTopoPoly();

	FaceNormalCache f2n;

	std::set<he::edge3*> checked_edges;

	auto first_edge = topo_roof->GetEdges().Head();
	auto curr_edge = first_edge;
	do {
		if (checked_edges.find(curr_edge) != checked_edges.end()) {
			curr_edge = curr_edge->linked_next;
			continue;
		}

		const auto& p0 = curr_edge->vert->position;
		const auto& p1 = curr_edge->next->vert->position;

		sm::mat4 mat;

		mat.Scale(sm::dis_pos3_to_pos3(p0, p1), 1.0f, 1.0f);

		bool is_valley = false;

		sm::vec3 norm;
		if (curr_edge->twin)
		{
			checked_edges.insert(curr_edge);
			checked_edges.insert(curr_edge->twin);

			auto n0 = f2n.GetNormal(curr_edge->loop);
			auto n1 = f2n.GetNormal(curr_edge->twin->loop);
			norm = (n0 + n1).Normalized();

			if (n0.Cross(n1).Dot(p1 - p0) > 0) {
				is_valley = true;
			}
		}
		else
		{
			checked_edges.insert(curr_edge);

			norm = f2n.GetNormal(curr_edge->loop);
		}

		sm::vec3 dir_x = (p1 - p0).Normalized();
		sm::vec3 dir_y = norm;
		sm::vec3 dir_z = dir_x.Cross(dir_y);
		mat = sm::mat4(sm::mat3(dir_x, dir_y, dir_z)) * mat;

		mat.Translate(p0.x, p0.y, p0.z);

		if (curr_edge->twin) 
		{
			if (base_plane.GetDistance(p0) > SM_LARGE_EPSILON &&
				base_plane.GetDistance(p1) > SM_LARGE_EPSILON) {
				ridge.push_back(mat);
			} else {
				if (is_valley) {
					valley.push_back(mat);
				} else {
					hip.push_back(mat);
				}
			}
		} 
		else 
		{
			eave.push_back(mat);
		}

		curr_edge = curr_edge->linked_next;
	} while (curr_edge != first_edge);
}

sm::mat4 ScopeTools::CalcInsertMat(const sm::cube& aabb, const sm::mat4& geo_mat, const sm::mat4& scope_mat)
{
	// for faces
	auto size = aabb.Size();
	if (size.x == 0 || size.y == 0 || size.z == 0) {
		return scope_mat;
	}

	auto min = aabb.min;
	auto max = aabb.max;

	sm::cube new_aabb;
	new_aabb.Combine(geo_mat * sm::vec3(min[0], min[1], min[2]));
	new_aabb.Combine(geo_mat * sm::vec3(min[0], min[1], max[2]));
	new_aabb.Combine(geo_mat * sm::vec3(min[0], max[1], min[2]));
	new_aabb.Combine(geo_mat * sm::vec3(min[0], max[1], max[2]));
	new_aabb.Combine(geo_mat * sm::vec3(max[0], min[1], min[2]));
	new_aabb.Combine(geo_mat * sm::vec3(max[0], min[1], max[2]));
	new_aabb.Combine(geo_mat * sm::vec3(max[0], max[1], min[2]));
	new_aabb.Combine(geo_mat * sm::vec3(max[0], max[1], max[2]));

	size = new_aabb.Size();
	if (size.x == 0 || size.y == 0 || size.z == 0) {
		return scope_mat;
	}

	const float sx = 1.0f / size.x;
	const float sy = 1.0f / size.y;
	const float sz = 1.0f / size.z;

	auto mat_s = sm::mat4::Scaled(sx, sy, sz);

	auto mat_t = sm::mat4::Translated(-new_aabb.xmin, -new_aabb.ymin, -new_aabb.zmin);

	return scope_mat * mat_s * mat_t;
}

}