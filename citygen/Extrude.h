#pragma once

#include <SM_Vector.h>

#include <memory>
#include <map>

namespace gs { class Polygon2D; }
namespace pm3 { class Polytope; }

namespace citygen
{

class StraightSkeleton;

class Extrude
{
public:
	static std::shared_ptr<pm3::Polytope> 
		Face(const std::shared_ptr<gs::Polygon2D>& polygon, float distance);

	static std::shared_ptr<pm3::Polytope> 
		Skeleton(const std::shared_ptr<gs::Polygon2D>& polygon, float distance);

}; // Extrude

}