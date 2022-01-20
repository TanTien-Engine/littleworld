#pragma once

#include <SM_Vector.h>
#include <SM_Rect.h>

#include <vector>

namespace citygen
{

class Reshape
{
public:
	Reshape(const std::vector<sm::vec2>& border);

	std::vector<std::vector<sm::vec2>> 
		ShapeL(float front_width, float left_width, bool remainder) const;
	std::vector<std::vector<sm::vec2>>
		ShapeU(float front_width, float left_width, float right_width, bool remainder) const;
	std::vector<std::vector<sm::vec2>>
		ShapeO(float front_width, float back_width, float left_width, float right_width, bool remainder) const;

private:
	sm::vec2 CalcRectPos(const sm::vec2& relative) const;

private:
	std::vector<sm::vec2> m_border;

	std::pair<sm::rect, float> m_obb;

}; // Reshape

}