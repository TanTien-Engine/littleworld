#pragma once

#include <halfedge/HalfEdge.h>
#include <SM_Vector.h>

#include <map>
#include <memory>

namespace pm3 { class Polytope; }

namespace archgen
{

class FaceNormalCache
{
public:
	sm::vec3 GetNormal(const he::loop3* face);

private:
	std::map<const he::loop3*, sm::vec3> m_face2norm;

}; // FaceNormalCache

class PolytopeTools
{
public:
	static std::shared_ptr<pm3::Polytope> Offset(const pm3::Polytope& poly, float dist);

}; // PolytopeTools

}