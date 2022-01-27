#include "Chaikin.h"

namespace citygen
{

std::vector<sm::vec2> smooth_chaikin(const std::vector<sm::vec2>& polyline, size_t times)
{
	if (polyline.size() < 3) {
		return polyline;
	}

	std::vector<sm::vec2> prev = polyline;

	for (int i = 0; i < times; ++i)
	{
		std::vector<sm::vec2> curr;

		curr.push_back(prev.front());
		for (size_t j = 1, m = prev.size() - 1; j < m; ++j)
		{
			curr.push_back(prev[j - 1] * 0.25f + prev[j] * 0.75f);
			curr.push_back(prev[j + 1] * 0.25f + prev[j] * 0.75f);
		}
		curr.push_back(prev.back());

		prev = curr;
	}

	return prev;
}

}
