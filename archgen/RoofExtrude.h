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

	static std::shared_ptr<pm3::Polytope>
		Pyramid(const std::shared_ptr<pm3::Polytope>& poly, float distance);

	static std::shared_ptr<pm3::Polytope>
		Shed(const std::shared_ptr<pm3::Polytope>& poly, float distance, int index = 0);

	static std::shared_ptr<pm3::Polytope>
		Gable(const std::shared_ptr<pm3::Polytope>& poly, float distance);

}; // RoofExtrude

}