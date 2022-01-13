#pragma once

#include <SM_Vector.h>
#include <SM_Rect.h>

#include <vector>

namespace citygen
{

class RotatingCalipers
{
public:
	static std::pair<sm::rect, float> 
		CalcOBB(const std::vector<sm::vec2>& poly, bool loop);

}; // RotatingCalipers

}