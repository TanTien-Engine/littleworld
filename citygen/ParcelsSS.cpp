#include "ParcelsSS.h"
#include "Graph.h"
#include "Math.h"
#include "StraightSkeleton.h"

#include <SM_Calc.h>

namespace citygen
{

ParcelsSS::ParcelsSS(const std::vector<sm::vec2>& border)
	: m_border(border)
{
}

void ParcelsSS::Build(float max_len)
{
	m_max_len = max_len;

	m_ss = std::make_shared<StraightSkeleton>(m_border);
}

std::vector<std::vector<sm::vec2>> ParcelsSS::GetPolygons() const
{
	auto graph = std::make_shared<Graph>();

	auto mid_line = m_ss->TravelStroke(m_border);

	auto out_line = m_border;
	out_line.push_back(out_line.front());

	const float ang_skip = 0.5f;

	float curr_d = 0;
	for (int i = 0; i < out_line.size() - 1; ++i)
	{
		auto& p0 = out_line[i];
		auto& p1 = out_line[i + 1];

		float d = sm::dis_pos_to_pos(p0, p1);
		if (curr_d + d >= m_max_len)
		{
			auto p = p0 + (p1 - p0) / d * (m_max_len - curr_d);
			auto o = p + sm::rotate_vector((p1 - p).Normalized() * 10, SM_PI * 0.5f);

			sm::vec2 cross;
			size_t idx;
			if (sm::intersect_segment_polyline(p, o, mid_line, &cross, &idx)) 
			{
				const float MAX_ANG = SM_PI * 0.5f;

				auto v0 = (p1 - p0).Normalized();
				auto v1 = (mid_line[(idx + 1) % mid_line.size()] - mid_line[idx]).Normalized();
				if (fabs(v0.Dot(v1)) > 0.5f)
				{
					graph->AddPath({ p, cross });
					out_line.insert(out_line.begin() + i + 1, p);
					mid_line.insert(mid_line.begin() + idx + 1, cross);
				}
			} 

			curr_d = 0;
		}
		else
		{
			curr_d += d;
		}
	}

	Math::IntersectPaths(out_line, mid_line);

	graph->AddPath(out_line);
	graph->AddPath(mid_line);

	graph->BuildHalfedge();

	std::vector<std::vector<sm::vec2>> polys;
	for (auto& poly : graph->GetPolygons()) {
		if (!sm::is_polygon_clockwise(poly)) {
			polys.push_back(poly);
		}
	}
	return polys;
}

}