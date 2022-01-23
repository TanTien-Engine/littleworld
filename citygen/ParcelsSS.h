#pragma once

#include <SM_Vector.h>

#include <vector>
#include <memory>

namespace citygen
{

class StraightSkeleton;

class ParcelsSS
{
public:
	ParcelsSS(const std::vector<sm::vec2>& border);

	void Build(float max_len);

	std::vector<std::vector<sm::vec2>> GetPolygons() const;

	void SetSeed(float seed) { m_seed = seed; }

private:
	std::vector<sm::vec2> TravelStroke() const;

private:
	std::vector<sm::vec2> m_border;

	std::shared_ptr<StraightSkeleton> m_ss = nullptr;

	float m_seed = 0.0f;
	float m_max_len = 0.1f;

}; // ParcelsSS

}