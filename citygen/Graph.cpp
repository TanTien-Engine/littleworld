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
		v0->AddEdge(v1);
		v1->AddEdge(v0);
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

			auto v0 = e0->t;
			auto v1 = e1->t;

			for (auto e : v0->edges) {
				if (e->t == vert) {
					e->t = v1;
				}
			}
			for (auto e : v1->edges) {
				if (e->t == vert) {
					e->t = v0;
				}
			}

			delete vert;
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
	// clean halfedge
	for (auto& v : m_vertices) {
		for (auto& e : v->edges) {
			e->pair = nullptr;
			e->prev = nullptr;
			e->next = nullptr;
		}
	}
	// sort edges
	for (auto& v : m_vertices) {
		std::sort(v->edges.begin(), v->edges.end(), EdgeComp());
	}

	auto pair = [](Edge* e0, Edge* e1) {
		e0->pair = e1;
		e1->pair = e0;
	};
	auto conn = [](Edge* e0, Edge* e1) {
		e0->next = e1;
		e1->prev = e0;
	};

	for (auto itr = m_vertices.begin(); itr != m_vertices.end(); ++itr)
	{
		Vertex* vert = *itr;
		for (auto itr2 = vert->edges.begin(); itr2 != vert->edges.end(); ++itr2)
		{
			auto edge = *itr2;

			// pair
			if (!edge->pair) {
				for (auto itr3 = edge->t->edges.begin(); itr3 != edge->t->edges.end() && !edge->pair; ++itr3) {
					if ((*itr3)->t == vert) {
						pair(edge, *itr3);
						break;
					}
				}
			}

			// next and prev
			if (!edge->next)
			{
				if (edge->t->edges.size() == 2)
				{
					auto e0 = *edge->t->edges.begin();
					auto e1 = *(++edge->t->edges.begin());
					if (e0->t == vert) {
						conn(edge, e1);
					} else {
						conn(edge, e0);
					}
				}
				else
				{
					auto itr = edge->t->edges.begin();
					for (; itr != edge->t->edges.end(); ++itr)
					{
						if ((*itr)->t == vert) {
							break;
						}
					}

					assert(itr != edge->t->edges.end());
					auto next = itr == edge->t->edges.begin() ? --edge->t->edges.end() : --itr;

					conn(edge, *next);
				}
			}
		}
	}
}

Graph::Vertex* Graph::AddVertex(const sm::vec2& pos)
{
	for (auto& vert : m_vertices) 
	{
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

	// reset edge
	for (auto vert : m_vertices) {
		for (auto& edge : vert->edges) {
			edge->visited = false;
		}
	}

	for (auto vert : m_vertices)
	{
		for (auto& edge : vert->edges)
		{
			if (edge->visited) {
				continue;
			}

			std::vector<sm::vec2> block;

			const Edge* first = edge;
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

void Graph::VertexMerge(float dist)
{
	for (auto itr_v = m_vertices.begin(); itr_v != m_vertices.end(); )
	{
		Vertex* vert = *itr_v;

		Vertex* near_v = nullptr;
		float near_d = FLT_MAX;
		for (auto& edge : vert->edges)
		{
			float d = sm::dis_pos_to_pos(vert->pos, edge->t->pos);
			if (d < near_d) {
				near_d = d;
				near_v = edge->t;
			}
		}

		if (near_d < dist)
		{
			near_v->pos = (near_v->pos + vert->pos) / 2;

			// in edges
			for (auto& edge_t : vert->edges) 
			{
				for (auto itr_e = edge_t->t->edges.begin(); itr_e != edge_t->t->edges.end(); )
				{
					bool rm = false;

					auto& edge_f = *itr_e;
					if (edge_f->t == vert) 
					{
						if (edge_f->f == near_v) {
							rm = true;
						}
						for (auto e : edge_t->t->edges) {
							if (e->t == near_v) {
								rm = true;
								break;
							}
						}
						if (rm) {
							delete* itr_e;
							itr_e = edge_t->t->edges.erase(itr_e);
							rm = true;
						} else {
							edge_f->t = near_v;
						}
					}

					if (!rm) {
						++itr_e;
					}
				}
			}

			// out edges
			for (auto& edge : vert->edges)
			{
				if (near_v != edge->t) {
					near_v->AddEdge(edge->t);
				}
			}

			itr_v = m_vertices.erase(itr_v);
		}
		else
		{
			++itr_v;
		}
	}
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

bool Graph::EdgeComp::operator () (const Edge* lhs, const Edge* rhs) const
{
	float ang0 = sm::get_line_angle(lhs->f->pos, lhs->t->pos);
	float ang1 = sm::get_line_angle(rhs->f->pos, rhs->t->pos);
	return ang0 < ang1;
}

//////////////////////////////////////////////////////////////////////////
// class Graph::Vertex
//////////////////////////////////////////////////////////////////////////

Graph::Vertex::~Vertex()
{
	for (auto& e : edges) {
		delete e;
	}
}

void Graph::Vertex::AddEdge(Vertex* vert)
{
	for (auto e : edges) {
		if (e->t == vert) {
			return;
		}
	}
	edges.push_back(new Edge(this, vert));
}

}