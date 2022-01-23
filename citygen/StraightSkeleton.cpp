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

std::shared_ptr<StraightSkeleton::PathTree> 
StraightSkeleton::GetPathTree() const
{
	if (!m_path_tree) {
		BuildPathTree();
	}

	return m_path_tree;
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

	m_path_tree = std::make_shared<PathTree>();
	for (auto& itr : v2n) {
		m_path_tree->nodes.push_back(itr.second);
	}
	m_path_tree->edges = paths;
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

std::shared_ptr<StraightSkeleton::Path> 
StraightSkeleton::PathTree::GetRoot() const
{
	std::shared_ptr<StraightSkeleton::Path> root = nullptr;

	float max_dist = -FLT_MAX;
	for (auto& path : edges)
	{
		const float d = path->nodes[0]->dist + path->nodes[1]->dist;
		if (d > max_dist) {
			max_dist = d;
			root = path;
		}
	}

	return root;
}

}