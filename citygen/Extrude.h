#pragma once

#include <memory>

namespace gs { class Polygon2D; }
namespace pm3 { class Polytope; }

namespace citygen
{

class Extrude
{
public:
	static std::shared_ptr<pm3::Polytope> 
		Face(const std::shared_ptr<gs::Polygon2D>& polygon, float distance);

	static std::shared_ptr<pm3::Polytope> 
		Skeleton(const std::shared_ptr<gs::Polygon2D>& polygon, float distance);

}; // Extrude

}