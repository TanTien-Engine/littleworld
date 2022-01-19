#pragma once

#include <SM_Vector.h>

#include <vector>

namespace citygen
{

class Math
{
public:
	static void IntersectPaths(std::vector<sm::vec2>& p0, std::vector<sm::vec2>& p1);

	static std::vector<sm::vec2> PathCutBy(const std::vector<sm::vec2>& base, const std::vector<sm::vec2>& cut);

}; // Math

}