#include "Math.h"

#include <SM_Calc.h>

#include <map>

namespace citygen
{

void Math::IntersectPaths(std::vector<sm::vec2>& p0, std::vector<sm::vec2>& p1)
{
	p0 = PathCutBy(p0, p1);
	p1 = PathCutBy(p1, p0);
}

std::vector<sm::vec2> Math::PathCutBy(const std::vector<sm::vec2>& base, const std::vector<sm::vec2>& cut)
{
	if (base.empty()) {
		return std::vector<sm::vec2>();
	}

	std::map<float, sm::vec2> new_pts;

	// points
	for (size_t i = 0, n = base.size() - 1; i < n; ++i)
	{
		for (size_t j = 0, m = cut.size(); j < m; ++j)
		{
			if (sm::is_point_in_segment(cut[j], base[i], base[i + 1])) 
			{
				float d = sm::dis_pos_to_pos(cut[j], base[i]) / sm::dis_pos_to_pos(base[i], base[i + 1]);
				sm::vec2 new_p = base[i] + (base[i + 1] - base[i]) * d;

				new_pts.insert({ i + d, new_p });
			}
		}
	}

	// cross
	for (size_t i = 0, n = base.size() - 1; i < n; ++i)
	{
		for (size_t j = 0, m = cut.size() - 1; j < m; ++j)
		{
			sm::vec2 new_p;
			if (sm::intersect_segment_segment(base[i], base[i + 1], cut[j], cut[j + 1], &new_p)) 
			{
				bool exist = false;
				for (auto& np : new_pts) {
					if (sm::dis_pos_to_pos(np.second, new_p) < SM_LARGE_EPSILON) {
						exist = true;
						break;
					}
				}

				if (!exist) {
					float d = sm::dis_pos_to_pos(new_p, base[i]) / sm::dis_pos_to_pos(base[i], base[i + 1]);
					new_pts.insert({ i + d, new_p });
				}
			}
		}
	}

	std::vector<sm::vec2> ret;

	auto itr = new_pts.begin();

	for (size_t i = 0, n = base.size(); i < n; ++i)
	{
		ret.push_back(base[i]);

		while (itr != new_pts.end() && itr->first < i + 1)
		{
			ret.push_back(itr->second);
			itr++;
		}
	}

	return ret;
}

}