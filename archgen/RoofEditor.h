#pragma once

#include <memory>

namespace pm3 { class Polytope; }

namespace archgen
{

class RoofEditor
{
public:
	// extrude
	static std::shared_ptr<pm3::Polytope> 
		Hip(const std::shared_ptr<pm3::Polytope>& poly, float distance);
	static std::shared_ptr<pm3::Polytope>
		Pyramid(const std::shared_ptr<pm3::Polytope>& poly, float distance);
	static std::shared_ptr<pm3::Polytope>
		Shed(const std::shared_ptr<pm3::Polytope>& poly, float distance, int index = 0);
	static std::shared_ptr<pm3::Polytope>
		Gable(const std::shared_ptr<pm3::Polytope>& poly, float distance);

	// offset
	static std::shared_ptr<pm3::Polytope>
		Offset(const pm3::Polytope& roof, const pm3::Polytope& base, float distance);

}; // RoofEditor

}