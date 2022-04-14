#pragma once

#include <halfedge/HalfEdge.h>
#include <SM_Vector.h>

#include <map>

namespace archgen
{

class FaceNormalCache
{
public:
	sm::vec3 GetNormal(const he::loop3* face);

private:
	std::map<const he::loop3*, sm::vec3> m_face2norm;

}; // FaceNormalCache

}