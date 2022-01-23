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

	auto mid_line = TravelStroke();

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

std::vector<sm::vec2> ParcelsSS::TravelStroke() const
{
	auto path_tree = m_ss->GetPathTree();
	auto path_root = path_tree->GetRoot();
	assert(path_root);

	float border_width = FLT_MAX;

	auto travel = [&](const std::shared_ptr<StraightSkeleton::Node>& except) -> std::vector<sm::vec2>
	{
		std::vector<sm::vec2> path;

		std::shared_ptr<StraightSkeleton::Node> prev = except;
		std::shared_ptr<StraightSkeleton::Path> curr = path_root;
		while (curr)
		{
			auto v = curr->nodes[0] == prev ? curr->nodes[1] : curr->nodes[0];
			path.push_back(v->pos);

			assert(!v->paths.empty());
			if (v->paths.size() == 1) 
			{
				curr = nullptr;
			}
			else if (v->paths.size() == 2)
			{
				curr = v->paths[0] == curr ? v->paths[1] : v->paths[0];
			}
			else
			{
				std::vector<float> angles;
				for (auto& path : v->paths)
				{
					auto other = path->nodes[0] == v ? path->nodes[1] : path->nodes[0];
					angles.push_back(sm::get_angle(v->pos, prev->pos, other->pos));
				}

				float min_angle = FLT_MAX;
				int min_idx = -1;
				for (size_t i = 0, n = angles.size(); i < n; ++i) 
				{
					const float d = static_cast<float>(fabs(angles[i] - SM_PI));
					if (d < min_angle) {
						min_angle = d;
						min_idx = static_cast<int>(i);
					}
				}

				if (min_angle > SM_PI * 0.2f) 
				{
					sm::vec2 out = v->pos + (v->pos - prev->pos).Normalized() * 10;
					for (size_t i = 0, n = m_border.size(); i < n; ++i)
					{
						sm::vec2 cross;
						if (sm::intersect_segment_segment(m_border[i], m_border[(i + 1) % n], v->pos, out, &cross))
						{
							const float d = sm::dis_pos_to_pos(v->pos, cross);
							if (d < border_width) {
								border_width = d;
							}

							path.push_back(cross);
							break;
						}
					}

					curr = nullptr;
				} 
				else 
				{
					assert(min_idx != -1);
					curr = v->paths[min_idx];
				}
			}
			prev = v;
		}

		return path;
	};

	auto path0 = travel(path_root->nodes[1]);
	auto path1 = travel(path_root->nodes[0]);

	std::reverse(path0.begin(), path0.end());
	std::copy(path1.begin(), path1.end(), std::back_inserter(path0));
	return path0;
}

}