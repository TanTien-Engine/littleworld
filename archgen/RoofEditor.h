#pragma once

#include <polymesh3/Polytope.h>

#include <memory>
#include <vector>

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

private:
	static std::shared_ptr<pm3::Polytope>
		ShedWithHoles(const std::vector<pm3::Polytope::PointPtr>& points,
			const pm3::Polytope::FacePtr& face, const sm::vec3& normal, float distance);

}; // RoofEditor

}