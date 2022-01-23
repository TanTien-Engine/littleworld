#include "StraightSkeleton.h"
#include "Graph.h"

#include <SM_Calc.h>
#include <CGAL/create_offset_polygons_2.h>

#include <map>
#include <queue>
#include <iterator>

namespace citygen
{

StraightSkeleton::StraightSkeleton(const std::vector<sm::vec2>& border)
{
	m_poly = PolygonPtr(new Polygon_2);
	for (auto& p : border) {
		m_poly->push_back(K::Point_2(p.x, p.y));
	}

	m_in_skel = CGAL::create_interior_straight_skeleton_2(m_poly);
}

std::vector<std::vector<sm::vec2>> StraightSkeleton::Offset(float distance) const
{
    std::vector<std::vector<sm::vec2>> ret;

    float offset = -distance;
    if (offset < 0)
    {
        double d_offset = -offset;

        if (!m_in_skel) {
			m_in_skel = CGAL::create_interior_straight_skeleton_2(m_poly);
        }

        if (m_in_skel)
        {
            auto polys = CGAL::create_offset_polygons_2<Polygon_2>(d_offset, *m_in_skel);
            auto border = GetBorder(polys);
            std::copy(border.begin(), border.end(), std::back_inserter(ret));
        }
    }
    else
    {
        double d_offset = offset;

        if (!m_ex_skel) {
            m_ex_skel = CGAL::create_exterior_straight_skeleton_2(d_offset * 2, m_poly);
        }

        if (m_ex_skel)
        {
            auto polys = CGAL::create_offset_polygons_2<Polygon_2>(d_offset, *m_ex_skel);
            auto border = GetBorder(polys);
            std::copy(border.begin(), border.end(), std::back_inserter(ret));
        }
    }

    return ret;
}

std::vector<std::vector<sm::vec2>> StraightSkeleton::Faces() const
{
	std::vector<std::vector<sm::vec2>> ret;

    for (auto itr = m_in_skel->faces_begin(); itr != m_in_skel->faces_end(); ++itr)
    {
        std::vector<sm::vec2> pts;

        auto first = itr->halfedge();
        auto curr = first;
        do {
            auto p = curr->vertex()->point();
            pts.push_back(sm::vec2((float)p.x(), (float)p.y()));

            curr = curr->next();
        } while (curr != first);

		ret.push_back(pts);
    }

	return ret;
}

void StraightSkeleton::BuildPathTree() const
{
	auto graph = std::make_shared<Graph>();
	for (auto itr = m_in_skel->halfedges_begin(); itr != m_in_skel->halfedges_end(); ++itr)
	{
		if (itr->is_bisector() && (itr->id() % 2 == 0))
		{
			auto p1 = itr->vertex()->point();
			auto p2 = itr->opposite()->vertex()->point();

			graph->AddPath({ sm::vec2(p1.x(), p1.y()), sm::vec2(p2.x(), p2.y()) });
		}
	}

	auto verts = graph->GetVertices();

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

	m_path_tree_root = paths.empty() ? nullptr : paths.front();
}

std::vector<sm::vec2> StraightSkeleton::TravelStroke(const std::vector<sm::vec2>& border) const
{
	if (!m_path_tree_root) {
		BuildPathTree();
	}

	float border_width = FLT_MAX;

	auto travel = [&](const std::shared_ptr<Node>& except) -> std::vector<sm::vec2>
	{
		std::vector<sm::vec2> path;

		std::shared_ptr<Node> prev = except;
		std::shared_ptr<Path> curr = m_path_tree_root;
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
					for (size_t i = 0, n = border.size(); i < n; ++i)
					{
						sm::vec2 cross;
						if (sm::intersect_segment_segment(border[i], border[(i + 1) % n], v->pos, out, &cross))
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

	auto path0 = travel(m_path_tree_root->nodes[1]);
	auto path1 = travel(m_path_tree_root->nodes[0]);

	std::reverse(path0.begin(), path0.end());
	std::copy(path1.begin(), path1.end(), std::back_inserter(path0));
	return path0;
}

std::map<sm::vec2, float> StraightSkeleton::GetAllNodeDist() const
{
	if (!m_path_tree_root) {
		BuildPathTree();
	}

	std::map<sm::vec2, float> ret;

	std::queue<std::shared_ptr<Node>> buf;
	buf.push(m_path_tree_root->nodes[0]);
	buf.push(m_path_tree_root->nodes[1]);

	std::set<std::shared_ptr<Node>> old_set;
	old_set.insert(m_path_tree_root->nodes[0]);
	old_set.insert(m_path_tree_root->nodes[1]);

	while (!buf.empty())
	{
		auto n = buf.front(); buf.pop();

		// fixme
		float dist = n->dist;
		if (n == m_path_tree_root->nodes[0] || m_path_tree_root->nodes[1]) {
			dist = std::max(m_path_tree_root->nodes[0]->dist, m_path_tree_root->nodes[1]->dist);
		}
		ret.insert({ n->pos, dist });

		for (auto& p : n->paths) 
		{
			auto o = n == p->nodes[0] ? p->nodes[1] : p->nodes[0];
			if (old_set.find(o) == old_set.end()) {
				buf.push(o);
				old_set.insert(o);
			}
		}
	}

	return ret;
}

std::vector<std::vector<sm::vec2>> 
StraightSkeleton::GetBorder(const std::vector<PolygonPtr>& polys)
{
    std::vector<std::vector<sm::vec2>> ret;

    for (auto poly : polys)
    {
        std::vector<sm::vec2> loop;
        loop.reserve(poly->size());
        for (auto itr = poly->vertices_begin(); itr != poly->vertices_end(); ++itr) {
            sm::vec2 p(static_cast<float>(itr->x()), static_cast<float>(itr->y()));
            loop.push_back(p);
        }
        ret.push_back(loop);
    }

    return ret;
}

}