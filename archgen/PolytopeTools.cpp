#include "PolytopeTools.h"

#include <halfedge/Utility.h>

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

}