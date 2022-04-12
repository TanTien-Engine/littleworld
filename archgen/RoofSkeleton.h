#pragma once

#include <SM_Vector.h>

#include <map>
#include <vector>
#include <memory>

namespace citygen { class StraightSkeleton; }

namespace archgen
{

class RoofSkeleton
{
public:
	RoofSkeleton(const std::vector<sm::vec2>& polygon);

	auto& Faces() const { return m_faces; }

	float QueryDist(const sm::vec2& p) const;

	float GetMaxDist() const { return m_max_dist; }

private:
	void CalcSkelDist();

private:
	std::shared_ptr<citygen::StraightSkeleton> m_ss = nullptr;

	std::vector<std::vector<sm::vec2>> m_faces;

	std::map<sm::vec2, float> m_pos2dist;
	float m_max_dist = 0;

}; // RoofSkeleton

}