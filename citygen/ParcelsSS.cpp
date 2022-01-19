#include "ParcelsSS.h"
#include "Graph.h"

#include <SM_Calc.h>

#include <queue>

namespace citygen
{

ParcelsSS::ParcelsSS(const std::vector<sm::vec2>& border)
	: m_border(border)
{
}

void ParcelsSS::Build()
{
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

std::vector<std::vector<sm::vec2>> ParcelsSS::GetPolygons(float dist) const
{
    std::vector<std::vector<sm::vec2>> polys;

	
  //  for (auto itr = m_skel->halfedges_begin(); itr != m_skel->halfedges_end(); ++itr)
  //  {
		//if (itr->is_bisector() && (itr->id() % 2 == 0))
		//{
		//	auto p1 = itr->vertex()->point();
		//	auto p2 = itr->opposite()->vertex()->point();

		//	std::vector<sm::vec2> poly;
		//	poly.push_back(sm::vec2(p1.x(), p1.y()));
		//	poly.push_back(sm::vec2(p2.x(), p2.y()));
		//	polys.push_back(poly);
		//}		
  //  }


	//for (auto& v : m_graph->GetVertices())
	//{
	//	//if (v->edges.size() != 1)
	//	{
	//		for (auto& e : v->edges)
	//		{
	//			if (v->edges.size() > 1 && e->t->edges.size() > 1) {
	//				polys.push_back({ v->pos, e->t->pos });
	//			}
	//		}
	//	}
	//}

	for (int i = 0, n = std::min((int)m_paths.size(), (int)(dist * 100)); i < n; ++i)
	{
		polys.push_back(m_paths[i]->pts);
	}

	//for (auto& path : m_paths) 
	//{
	//	if (path->nodes[0]->dist + path->nodes[1]->dist > dist)
	//	{
	//		polys.push_back(path->pts);
	//	}
	//}

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
	m_paths = paths;
}

}