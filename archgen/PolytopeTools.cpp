#include "PolytopeTools.h"
#include "../citygen/PolyBuilder.h"
#include "../citygen/StraightSkeleton.h"

#include <polymesh3/Polytope.h>
#include <halfedge/Utility.h>
#include <SM_Polyline.h>

#define USE_CGAL

namespace archgen
{

sm::vec3 FaceNormalCache::GetNormal(const he::loop3* face)
{
	auto itr = m_face2norm.find(face);
	if (itr != m_face2norm.end())
	{
		return itr->second;
	} 
	else 
	{
		sm::Plane plane;
		he::Utility::LoopToPlane(*face, plane);
		auto& n = plane.normal;

		m_face2norm.insert({ face, n });
		return n;
	}
}

std::shared_ptr<pm3::Polytope> 
PolytopeTools::Offset(const pm3::Polytope& poly, float dist)
{
	auto& faces = poly.Faces();
	if (faces.size() != 1) {
		return nullptr;
	}

	auto& pts = poly.Points();
	auto& border = faces.front()->border;

	const float y = pts[border.front()]->pos.y;

	std::vector<sm::vec2> polyline;
	polyline.reserve(border.size());
	for (auto& i : border) {
		auto& p = pts[i]->pos;
		polyline.push_back(sm::vec2(p.x, p.z));
	}
	std::reverse(polyline.begin(), polyline.end());

#ifdef USE_CGAL
	citygen::StraightSkeleton ss(polyline);
	auto loop2 = ss.Offset(dist);
#else
	auto loop2 = sm::polyline_offset(polyline, dist, true);
#endif // USE_CGAL
	if (loop2.size() == 0 || loop2.back().size() < 3) {
		return nullptr;
	}

	std::vector<sm::vec3> loop3;
	loop3.reserve(loop2.back().size());
	for (auto& p : loop2.back()) {
		loop3.push_back(sm::vec3(p.x, y, p.y));
	}

	citygen::PolyBuilder builder;
#ifndef USE_CGAL
	std::reverse(loop3.begin(), loop3.end());
#endif // USE_CGAL
	builder.AddFace(loop3);

	auto polytope = builder.CreatePolytope();

	polytope->GetTopoPoly();
	polytope->BuildFromTopo();

	return polytope;
}

}