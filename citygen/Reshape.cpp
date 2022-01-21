#include "Reshape.h"
#include "RotatingCalipers.h"

#include <geoshape/Polygon2D.h>
#include <SM_Polygon.h>
#include <SM_Calc.h>
#include <sm_const.h>

namespace citygen
{

Reshape::Reshape(const std::shared_ptr<gs::Polygon2D>& polygon)
	: m_polygon(polygon)
{
	m_obb = RotatingCalipers::CalcOBB(m_polygon->GetVertices(), true);
}

std::vector<std::vector<sm::vec2>> 
Reshape::ShapeL(float front_width, float left_width, bool remainder) const
{
	std::vector<sm::vec2> clip;

	auto& r = m_obb.first;
	if (r.Width() > r.Height())
	{
		clip.push_back(CalcRectPos({ left_width, front_width }));
		clip.push_back(CalcRectPos({ FLT_MAX,    front_width }));
		clip.push_back(CalcRectPos({ FLT_MAX,    FLT_MAX }));
		clip.push_back(CalcRectPos({ left_width, FLT_MAX }));
	}
	else
	{
		clip.push_back(CalcRectPos({ -FLT_MAX,     left_width }));
		clip.push_back(CalcRectPos({ -front_width, left_width }));
		clip.push_back(CalcRectPos({ -front_width, FLT_MAX }));
		clip.push_back(CalcRectPos({ -FLT_MAX,     FLT_MAX }));
	}

	std::vector<sm::vec2> clip2 = {
		sm::rotate_vector(sm::vec2(r.xmin, r.ymin), m_obb.second),
		sm::rotate_vector(sm::vec2(r.xmax, r.ymin), m_obb.second),
		sm::rotate_vector(sm::vec2(r.xmax, r.ymax), m_obb.second),
		sm::rotate_vector(sm::vec2(r.xmin, r.ymax), m_obb.second),
	};

	return remainder ? 
		sm::polygon_intersection({ m_polygon->GetVertices() }, clip) : 
		sm::polygon_difference({ m_polygon->GetVertices() }, clip);
}

std::vector<std::vector<sm::vec2>>
Reshape::ShapeU(float front_width, float left_width, float right_width, bool remainder) const
{
	std::vector<sm::vec2> clip;

	auto& r = m_obb.first;
	if (r.Width() > r.Height())
	{
		clip.push_back(CalcRectPos({   left_width, front_width }));
		clip.push_back(CalcRectPos({ -right_width, front_width }));
		clip.push_back(CalcRectPos({ -right_width,     FLT_MAX }));
		clip.push_back(CalcRectPos({   left_width,     FLT_MAX }));
	}
	else
	{
		clip.push_back(CalcRectPos({     -FLT_MAX,   left_width }));
		clip.push_back(CalcRectPos({ -front_width,   left_width }));
		clip.push_back(CalcRectPos({ -front_width, -right_width }));
		clip.push_back(CalcRectPos({     -FLT_MAX, -right_width }));
	}

	return remainder ?
		sm::polygon_intersection({ m_polygon->GetVertices() }, clip) :
		sm::polygon_difference({ m_polygon->GetVertices() }, clip);
}

std::vector<std::vector<sm::vec2>>
Reshape::ShapeO(float front_width, float back_width, float left_width, float right_width, bool remainder) const
{
	std::vector<sm::vec2> clip;

	auto& r = m_obb.first;
	if (r.Width() > r.Height())
	{
		clip.push_back(CalcRectPos({   left_width, front_width }));
		clip.push_back(CalcRectPos({ -right_width, front_width }));
		clip.push_back(CalcRectPos({ -right_width, -back_width }));
		clip.push_back(CalcRectPos({   left_width, -back_width }));
	}
	else
	{
		clip.push_back(CalcRectPos({   back_width,   left_width }));
		clip.push_back(CalcRectPos({ -front_width,   left_width }));
		clip.push_back(CalcRectPos({ -front_width, -right_width }));
		clip.push_back(CalcRectPos({   back_width, -right_width }));
	}

	return remainder ?
		sm::polygon_intersection({ m_polygon->GetVertices() }, clip) :
		sm::polygon_difference({ m_polygon->GetVertices() }, clip);
}

sm::vec2 Reshape::CalcRectPos(const sm::vec2& relative) const
{
	auto to_absolute = [](float min, float max, float relative) -> float
	{
		if (relative == -FLT_MAX) {
			return min;
		} else if (relative == FLT_MAX) {
			return max;
		} else if (relative >= 0) {
			return min + relative;
		} else {
			return max + relative;
		}
	};

	auto& r = m_obb.first;
	float x = to_absolute(r.xmin - SM_LARGE_EPSILON, r.xmax + SM_LARGE_EPSILON, relative.x);
	float y = to_absolute(r.ymin - SM_LARGE_EPSILON, r.ymax + SM_LARGE_EPSILON, relative.y);

	return sm::rotate_vector(sm::vec2(x, y), m_obb.second);
}

}