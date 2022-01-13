#pragma once

#include <SM_Vector.h>

#include <vector>

namespace citygen
{

class KMeans
{
public:
	KMeans(const std::vector<sm::vec2>& pts);

	std::vector<std::vector<size_t>> Clustering(int k, float threshold) const;

private:
	bool NeedRecluster(const std::vector<sm::vec2>& centers, 
		const std::vector<std::vector<size_t>>& groups, float threshold) const;

	sm::vec2 CalcCenter(const std::vector<size_t>& pts) const;
	std::vector<sm::vec2> CalcCenters(const std::vector<std::vector<size_t>>& groups) const;

	std::vector<std::vector<size_t>> ClusterToPos(const std::vector<sm::vec2>& centers) const;

private:
	const std::vector<sm::vec2>& m_points;

}; // KMeans

}