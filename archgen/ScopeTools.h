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

	static void CalcRoofEdgesMapping(const pm3::Polytope& roof, const pm3::Polytope& base,
		std::vector<sm::mat4>& eave, std::vector<sm::mat4>& hip,
		std::vector<sm::mat4>& valley, std::vector<sm::mat4>& ridge);

	static sm::mat4 CalcInsertMat(const sm::cube& aabb, const sm::mat4& scope);

}; // ScopeTools

}