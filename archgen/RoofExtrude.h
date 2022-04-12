#pragma once

#include <memory>

namespace pm3 { class Polytope; }

namespace archgen
{

class RoofExtrude
{
public:
	static std::shared_ptr<pm3::Polytope> 
		Hip(const std::shared_ptr<pm3::Polytope>& poly, float distance);

}; // RoofExtrude

}