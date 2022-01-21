#include "ParcelsSS.h"
#include "Graph.h"
#include "Math.h"

#include <SM_Calc.h>
#include <sm_const.h>
#include <SM_Polyline.h>

#include <queue>

namespace citygen
{

ParcelsSS::ParcelsSS(const std::vector<sm::vec2>& border)
	: m_border(border)
{
}

void ParcelsSS::Build(float max_len)
{
	m_max_len = max_len;

	m_poly = PolygonPtr(new Polygon_2);
	for (auto& p : m_border) {
		m_poly->push_back(K::Point_2(p.x, p.y));
	}

	m_skel = CGAL::create_interior_straight_skeleton_2(m_poly);

	m_graph = std::make_shared<Graph>();
	for (auto itr = m_skel->halfedges_begin(); itr != m_skel->halfedges_end(); ++itr)
	{
		if (itr->is_bisector() && (itr->id() % 2 == 0))
		{
			auto p1 = itr->vertex()->point();
			auto p2 = itr->opposite()->vertex()->point();

			m_graph->AddPath({ sm::vec2(p1.x(), p1.y()), sm::vec2(p2.x(), p2.y()) });
		}
	}

	//std::vector<int> num(10, 0);
	//for (auto& vert : m_graph->GetVertices()) {
	//	++num[vert->edges.size()];
	//}

	BuildGraph();
}

std::vector<std::vector<sm::vec2>> ParcelsSS::GetPolygons() const
{
	auto graph = std::make_shared<Graph>();

	float border_width = FLT_MAX;
	auto mid_line = TravelSkeleton(border_width);

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

void ParcelsSS::BuildGraph()
{
	auto verts = m_graph->GetVertices();

	std::map<Graph::Vertex*, std::shared_ptr<Node>> v2n;

	auto new_node = [&](Graph::Vertex* vert) -> std::shared_ptr<Node>
	{
		std::shared_ptr<Node> node = nullptr;
		auto itr = v2n.find(vert);
		if (itr == v2n.end()) 
		{
			auto node = std::make_shared<Node>();
			node->pos = vert->pos;
			v2n.insert({ vert, node });
			return node;
		} 
		else 
		{
			return itr->second;
		}
	};

	std::vector<std::shared_ptr<Path>> paths;

	for (auto& vert : verts)
	{
		assert(!vert->edges.empty());
		if (vert->edges.size() == 2) {
			continue;
		}

		auto f_node = new_node(vert);
		for (auto& edge : vert->edges)
		{
			std::vector<sm::vec2> pts;

			Graph::Vertex* prev_v = vert;

			Graph::Edge* curr = edge;
			pts.push_back(edge->f->pos);
			while (true)
			{
				assert(!curr->t->edges.empty());
				pts.push_back(curr->t->pos);
				if (curr->t->edges.size() != 2) {
					break;
				}

				if (curr->t->edges[0]->t == prev_v) {
					curr = curr->t->edges[1];
				} else {
					curr = curr->t->edges[0];
				}
				prev_v = curr->f;
			}

			auto t_node = new_node(curr->t);

			bool find_path = false;
			for (auto& path : f_node->paths) 
			{
				auto& n2 = path->nodes;
				if (n2[0] == f_node && n2[1] == t_node ||
					n2[0] == t_node && n2[1] == f_node) {
					find_path = true;
					break;
				}
			}

			if (!find_path) 
			{
				auto path = std::make_shared<Path>();
				path->pts = pts;
				path->nodes[0] = f_node;
				path->nodes[1] = t_node;
				f_node->paths.push_back(path);
				t_node->paths.push_back(path);

				paths.push_back(path);
			}
		}
	}

	struct NodeCmp
	{
		bool operator()(std::shared_ptr<Node> a, std::shared_ptr<Node> b) const { 
			return a->dist > b->dist; 
		}
	};

	std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, NodeCmp> buf;
	for (auto pair : v2n) 
	{
		if (pair.second->paths.size() == 1) {
			pair.second->dist = 0;
		}
		buf.push(pair.second);
	}
	while (!buf.empty())
	{
		auto v = buf.top(); buf.pop();

		for (auto path : v->paths)
		{
			auto other = path->nodes[0] == v ? path->nodes[1] : path->nodes[0];

			float d = sm::dis_pos_to_pos(v->pos, other->pos);
			if (v->dist + d < other->dist) {
				other->dist = v->dist + d;
			}
		}
	}

	struct PathCmp
	{
		bool operator()(std::shared_ptr<Path> a, std::shared_ptr<Path> b) const {
			return a->nodes[0]->dist + a->nodes[1]->dist 
			     > b->nodes[0]->dist + b->nodes[1]->dist;
		}
	};
	std::sort(paths.begin(), paths.end(), PathCmp());

	m_root = paths.empty() ? nullptr : paths.front();
}

std::vector<sm::vec2> ParcelsSS::TravelSkeleton(float& border_width) const
{
	if (!m_root) {
		return {};
	}

	auto travel = [&](const std::shared_ptr<Node>& except) -> std::vector<sm::vec2>
	{
		std::vector<sm::vec2> path;

		std::shared_ptr<Node> prev = except;
		std::shared_ptr<Path> curr = m_root;
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
				for (int i = 0, n = angles.size(); i < n; ++i) 
				{
					const float d = fabs(angles[i] - SM_PI);
					if (d < min_angle) {
						min_angle = d;
						min_idx = i;
					}
				}

				if (min_angle > SM_PI * 0.2f) 
				{
					sm::vec2 out = v->pos + (v->pos - prev->pos).Normalized() * 10;
					for (int i = 0, n = m_border.size(); i < n; ++i)
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

	auto path0 = travel(m_root->nodes[1]);
	auto path1 = travel(m_root->nodes[0]);

	std::reverse(path0.begin(), path0.end());
	std::copy(path1.begin(), path1.end(), std::back_inserter(path0));
	return path0;
}

}