#include "Graph.h"

#include <SM_Calc.h>

namespace citygen
{

Graph::~Graph()
{
	for (auto v : m_vertices) {
		delete v;
	}
}

void Graph::AddPath(const std::vector<sm::vec2>& path)
{
	std::vector<Vertex*> m_vertices;

	for (auto& pos : path) 
	{
		auto vert = AddVertex(pos);
		if (m_vertices.empty() || m_vertices.back() != vert) {
			m_vertices.push_back(vert);
		}
	}

	if (m_vertices.size() < 2) {
		return;
	}

	for (size_t i = 0, n = m_vertices.size() - 1; i < n; ++i) 
	{
		auto v0 = m_vertices[i];
		auto v1 = m_vertices[i + 1];
		v0->edges.insert(Edge(v0, v1));
		v1->edges.insert(Edge(v1, v0));
	}
}

void Graph::RemoveDegTwoVert()
{
	for (auto itr = m_vertices.begin(); itr != m_vertices.end(); )
	{
		Vertex* vert = *itr;
		if (vert->edges.size() == 2) 
		{
			auto e0 = *vert->edges.begin();
			auto e1 = *(++vert->edges.begin());

			auto v0 = e0.t;
			auto v1 = e1.t;

			for (auto e : v0->edges) {
				if (e.t == vert) {
					e.t = v1;
				}
			}
			for (auto e : v1->edges) {
				if (e.t == vert) {
					e.t = v0;
				}
			}

			itr = m_vertices.erase(itr);
		}
		else
		{
			++itr;
		}
	}
}

void Graph::BuildHalfedge()
{
	auto pair = [](Edge& e0, Edge& e1) {
		e0.pair = &e1;
		e1.pair = &e0;
	};
	auto conn = [](Edge& e0, Edge& e1) {
		e0.next = &e1;
		e1.prev = &e0;
	};

	for (auto itr = m_vertices.begin(); itr != m_vertices.end(); ++itr)
	{
		Vertex* vert = *itr;
		for (auto itr2 = vert->edges.begin(); itr2 != vert->edges.end(); ++itr2)
		{
			Edge& edge = const_cast<Edge&>(*itr2);

			// pair
			if (!edge.pair) {
				for (auto itr3 = edge.t->edges.begin(); itr3 != edge.t->edges.end() && !edge.pair; ++itr3) {
					if (itr3->t == vert) {
						pair(edge, const_cast<Edge&>(*itr3));
						break;
					}
				}
			}

			// next and prev
			if (!edge.next)
			{
				if (edge.t->edges.size() == 2)
				{
					auto& e0 = *edge.t->edges.begin();
					auto& e1 = *(++edge.t->edges.begin());
					if (e0.t == vert) {
						conn(edge, const_cast<Edge&>(e1));
					} else {
						conn(edge, const_cast<Edge&>(e0));
					}
				}
				else
				{
					auto itr = edge.t->edges.begin();
					for (; itr != edge.t->edges.end(); ++itr)
					{
						if (itr->t == vert) {
							break;
						}
					}

					assert(itr != edge.t->edges.end());

					auto next = ++itr;
					if (next == edge.t->edges.end()) {
						next = edge.t->edges.begin();
					}

					//auto next = itr == edge.t->edges.begin() ? --edge.t->edges.end() : --itr;

					conn(edge, const_cast<Edge&>(*next));
				}
			}
		}
	}
}

Graph::Vertex* Graph::AddVertex(const sm::vec2& pos)
{
	for (auto& vert : m_vertices) {
		if (sm::dis_pos_to_pos(vert->pos, pos) < SM_LARGE_EPSILON) {
			return vert;
		}
	}

	Vertex* vert = new Vertex(pos);
	m_vertices.insert(vert);

	return vert;
}

std::vector<std::vector<sm::vec2>> Graph::GetPolygons() const
{
	std::vector<std::vector<sm::vec2>> polys;

	for (auto vert : m_vertices)
	{
		for (auto& edge : vert->edges)
		{
			if (edge.visited) {
				continue;
			}

			std::vector<sm::vec2> block;

			const Edge* first = &edge;
			const Edge* curr = first;
			do
			{
				curr->visited = true;

				block.push_back(curr->f->pos);

				curr = curr->next;
			} while (curr != first);

			polys.push_back(block);
		}
	}

	return polys;
}

//////////////////////////////////////////////////////////////////////////
// class Graph::VertexComp
//////////////////////////////////////////////////////////////////////////

bool Graph::VertexComp::operator () (const Vertex* lhs, const Vertex* rhs) const
{
	if (lhs->pos.y == rhs->pos.y) {
		return lhs->pos.x < rhs->pos.x;
	}
	else {
		return lhs->pos.y < rhs->pos.y;
	}
}

//////////////////////////////////////////////////////////////////////////
// class Graph::EdgeComp
//////////////////////////////////////////////////////////////////////////

bool Graph::EdgeComp::operator () (const Edge& lhs, const Edge& rhs) const
{
	float ang0 = sm::get_line_angle(lhs.f->pos, lhs.t->pos);
	float ang1 = sm::get_line_angle(rhs.f->pos, rhs.t->pos);
	return ang0 < ang1;
}

}