#pragma once

#include <polymesh3/Polytope.h>
#include <SM_Matrix.h>

namespace archgen
{

class ScopeTools
{
public:
	static bool CalcFaceMapping(const pm3::Polytope& poly,
		const pm3::Polytope::Face& face, sm::vec3& o, sm::vec3& x, sm::vec3& y);

	static std::vector<sm::mat4> CalcEdgesMapping(const pm3::Polytope& poly);
	static std::vector<sm::mat4> CalcFaceEdgesMapping(const pm3::Polytope& poly);

	static sm::mat4 CalcInsertMat(const sm::cube& aabb, const sm::mat4& scope);

}; // ScopeTools

}