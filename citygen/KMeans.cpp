#include "KMeans.h"
#include "Random.h"

#include <SM_Calc.h>

namespace citygen
{

KMeans::KMeans(const std::vector<sm::vec2>& pts)
	: m_points(pts)
{
}

std::vector<std::vector<size_t>> 
KMeans::Clustering(int k, float threshold) const
{
	std::vector<sm::vec2> centers(m_points);
	Random::RandomPermutation(centers);
	centers.erase(centers.begin() + k, centers.end());

	auto groups = ClusterToPos(centers);
	while (NeedRecluster(centers, groups, threshold))
	{
		centers = CalcCenters(groups);
		groups = ClusterToPos(centers);
	}

	return groups;
}

bool KMeans::NeedRecluster(const std::vector<sm::vec2>& centers,
	                       const std::vector<std::vector<size_t>>& groups,
	                       float threshold) const
{
	assert(centers.size() == groups.size());
	if (centers.empty()) {
		return false;
	}

	for (size_t i = 0; i < centers.size(); ++i)
	{
		assert(groups[i].size() > 0);
		auto center = CalcCenter(groups[i]);
		if (fabs(center.x - centers[i].x) > threshold ||
			fabs(center.y - centers[i].y) > threshold) {
			return true;
		}
	}

	return false;
}

sm::vec2 KMeans::CalcCenter(const std::vector<size_t>& pts) const
{
	if (pts.empty()) {
		return sm::vec2();
	}

	sm::vec2 center;
	for (auto& p : pts) {
		center += m_points[p];
	}
	center /= static_cast<float>(pts.size());

	return center;
}

std::vector<sm::vec2> KMeans::CalcCenters(const std::vector<std::vector<size_t>>& groups) const
{
	std::vector<sm::vec2> centers(groups.size());
	for (size_t i = 0; i < groups.size(); ++i) {
		centers[i] = CalcCenter(groups[i]);
	}
	return centers;
}

std::vector<std::vector<size_t>> KMeans::ClusterToPos(const std::vector<sm::vec2>& centers) const
{
	std::vector<std::vector<size_t>> groups;
	if (centers.empty()) {
		return groups;
	}

	groups.resize(centers.size());
	for (size_t i = 0, n = m_points.size(); i < n; ++i)
	{
		float min_dist = sm::dis_square_pos_to_pos(m_points[i], centers[0]);
		size_t i_nearest = 0;
		for (size_t j = 1, m = centers.size(); j < m; ++j)
		{
			float dist = sm::dis_square_pos_to_pos(m_points[i], centers[j]);
			if (dist < min_dist) {
				min_dist = dist;
				i_nearest = j;
			}
		}
		groups[i_nearest].push_back(i);
	}

	return groups;
}

}