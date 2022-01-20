#pragma once

#include <SM_Vector.h>
#include <SM_Rect.h>

#include <vector>
#include <memory>

namespace gs { class Polygon2D; }

namespace citygen
{

class Reshape
{
public:
	Reshape(const std::shared_ptr<gs::Polygon2D>& polygon);

	std::vector<std::vector<sm::vec2>> 
		ShapeL(float front_width, float left_width, bool remainder) const;
	std::vector<std::vector<sm::vec2>>
		ShapeU(float front_width, float left_width, float right_width, bool remainder) const;
	std::vector<std::vector<sm::vec2>>
		ShapeO(float front_width, float back_width, float left_width, float right_width, bool remainder) const;

private:
	sm::vec2 CalcRectPos(const sm::vec2& relative) const;

private:
	std::shared_ptr<gs::Polygon2D> m_polygon = nullptr;

	std::pair<sm::rect, float> m_obb;

}; // Reshape

}