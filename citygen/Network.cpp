#include "Network.h"
#include "TensorField.h"

#include <sm/SM_DouglasPeucker.h>
#include <SM_Calc.h>

#include <iterator>
#include <map>

namespace citygen
{

Network::Network(const std::shared_ptr<TensorField>& tf)
	: m_tf(tf)
{
}

void Network::BuildStreamlines(int num)
{
	float min = 0.0f;
	float max = 1.0f;
	m_border = {
		{ min, min },
		{ max, min },
		{ max, max },
		{ min, max },
		{ min, min },
	};

	auto w = m_tf->GetWidth();
	auto h = m_tf->GetHeight();
	
	for (size_t iy = 0; iy < num; ++iy) 
	{
		int y = static_cast<int>(h / num * (0.5f + iy));
		for (size_t ix = 0; ix < num; ++ix) 
		{
			int x = static_cast<int>(w / num * (0.5f + ix));
			
			m_major_paths.push_back(BuildPath(sm::ivec2(x, y), true));
			m_minor_paths.push_back(BuildPath(sm::ivec2(x, y), false));
		}
	}
}

void Network::BuildTopology()
{
	IntersectPaths();
	BuildGraph();
}

std::vector<sm::vec2> Network::GetNodes() const
{
	std::vector<sm::vec2> ret;
	for (auto& v : m_graph->vertices) {
		if (v->edges.size() != 2) {
			ret.push_back(v->pos);
		}
	}
	return ret;
}

std::vector<std::vector<sm::vec2>> Network::GetPolygons() const
{
	std::vector<std::vector<sm::vec2>> blocks;

	for (auto vert : m_graph->vertices)
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

			blocks.push_back(block);
		}
	}

	return blocks;
}

std::vector<sm::vec2> Network::BuildPath(const sm::ivec2& p, bool major) const
{
	auto f = Travel(p, major, true);
	auto b = Travel(p, major, false);

	auto points = b;
	std::reverse(points.begin(), points.end());
	std::copy(f.begin(), f.end(), std::back_inserter(points));

	std::vector<sm::vec2> path;
	sm::douglas_peucker(points, 0.5f, path);

	// to 0-1
	auto w = m_tf->GetWidth();
	auto h = m_tf->GetHeight();
	for (auto& p : path) {
		p.x /= w;
		p.y /= h;
	}

	return path;
}

std::vector<sm::vec2> Network::Travel(const sm::ivec2& p, bool major, bool forward) const
{
	std::vector<sm::vec2> points;

	sm::vec2 fp((float)p.x, (float)p.y);
	sm::vec2 prev_dir = sm::vec2(0, 0);

	const int max_steps = m_tf->GetWidth() * 2;
	for (size_t i = 0; i < max_steps; ++i)
	{
		points.push_back(fp);

		int x = int(fp.x);
		int y = int(fp.y);
//		out_color[y * map_size + x] = color;

		auto dir = CalcDir(sm::ivec2(x, y), major);
		if (!forward) {
			dir = -dir;
		}
		if (prev_dir != sm::vec2(0, 0) && prev_dir.Dot(dir) < 0) {
			dir = -dir;
		}
		prev_dir = dir;

		if (dir.x == 0 && dir.y == 0) {
//			out_color[y * map_size + x] = 255;
			break;
		}
		fp += dir;
	}

	return points;
}

sm::vec2 Network::CalcDir(const sm::ivec2& p, bool major) const
{
	if (p.x < 0 || p.x > m_tf->GetWidth() ||
		p.y < 0 || p.y > m_tf->GetHeight()) {
		return sm::vec2(0, 0);
	}

	auto t = m_tf->GetTensor(p.x, p.y);
	if (abs(t.x) < 0.00001) {
		return sm::vec2(0, 0);
	}

	float theta = atan2f(t.y, t.x) / 2.0f;
	if (!major) {
		theta += 1.57f;
	}
	return sm::vec2(cosf(theta), sinf(theta));
}

void Network::IntersectPaths()
{
	for (size_t i = 0; i < m_major_paths.size(); ++i) {
		IntersectPaths(m_border, m_major_paths[i]);
	}
	for (size_t i = 0; i < m_minor_paths.size(); ++i) {
		IntersectPaths(m_border, m_minor_paths[i]);
	}

	for (size_t i = 0; i < m_major_paths.size(); ++i) {
		for (size_t j = 0; j < m_minor_paths.size(); ++j) {
			IntersectPaths(m_major_paths[i], m_minor_paths[j]);
		}
	}
}

void Network::IntersectPaths(std::vector<sm::vec2>& p0, std::vector<sm::vec2>& p1)
{
	p0 = PathCutBy(p0, p1);
	p1 = PathCutBy(p1, p0);
}

std::vector<sm::vec2> Network::PathCutBy(const std::vector<sm::vec2>& base, const std::vector<sm::vec2>& cut)
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
				float dist = sm::dis_pos_to_pos(cut[j], base[i]);

				float tot = sm::dis_pos_to_pos(base[i], base[i + 1]);
				sm::vec2 new_p = base[i] + (base[i + 1] - base[i]) / tot * dist;

				new_pts.insert({ i + dist, new_p });
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
				float d = sm::dis_pos_to_pos(new_p, base[i]) / sm::dis_pos_to_pos(base[i], base[i + 1]);
				new_pts.insert({ i + d, new_p });
			}
		}
	}

	auto itr = new_pts.begin();

	std::vector<sm::vec2> ret;
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

void Network::BuildGraph()
{
	auto g = std::make_shared<Graph>();

	g->AddPath(m_border);
	for (auto& path : m_major_paths) {
		g->AddPath(path);
	}
	for (auto& path : m_minor_paths) {
		g->AddPath(path);
	}

	std::vector<int> num(10, 0);
	for (auto itr = g->vertices.begin(); itr != g->vertices.end(); ++itr) {
		++num[(*itr)->edges.size()];
	}

	//g->RemoveDegTwoVert();
	g->BuildHalfedge();
	
	m_graph = g;
}

//////////////////////////////////////////////////////////////////////////
// class Network::VertexComp
//////////////////////////////////////////////////////////////////////////

bool Network::VertexComp::operator () (const Vertex* lhs, const Vertex* rhs) const
{
	if (lhs->pos.y == rhs->pos.y) {
		return lhs->pos.x < rhs->pos.x;
	} else {
		return lhs->pos.y < rhs->pos.y;
	}
}

//////////////////////////////////////////////////////////////////////////
// class Network::EdgeComp
//////////////////////////////////////////////////////////////////////////

bool Network::EdgeComp::operator () (const Edge& lhs, const Edge& rhs) const
{
	float ang0 = sm::get_line_angle(lhs.f->pos, lhs.t->pos);
	float ang1 = sm::get_line_angle(rhs.f->pos, rhs.t->pos);
	return ang0 < ang1;
}

//////////////////////////////////////////////////////////////////////////
// class Network::Graph
//////////////////////////////////////////////////////////////////////////

Network::Graph::~Graph()
{
	for (auto v : vertices) {
		delete v;
	}
}

Network::Vertex* Network::Graph::AddVertex(const sm::vec2& pos)
{
	for (auto& vert : vertices) {
		if (sm::dis_pos_to_pos(vert->pos, pos) < SM_LARGE_EPSILON) {
			return vert;
		}
	}

	Vertex* vert = new Vertex(pos);
	vertices.insert(vert);

	return vert;
}

void Network::Graph::AddPath(const std::vector<sm::vec2>& path)
{
	std::vector<Vertex*> vertices;

	for (auto& pos : path) 
	{
		auto vert = AddVertex(pos);
		if (vertices.empty() || vertices.back() != vert) {
			vertices.push_back(vert);
		}
	}

	if (vertices.size() < 2) {
		return;
	}

	for (size_t i = 0, n = vertices.size() - 1; i < n; ++i) 
	{
		auto v0 = vertices[i];
		auto v1 = vertices[i + 1];
		v0->edges.insert(Edge(v0, v1));
		v1->edges.insert(Edge(v1, v0));
	}
}

void Network::Graph::RemoveDegTwoVert()
{
	for (auto itr = vertices.begin(); itr != vertices.end(); )
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

			itr = vertices.erase(itr);
		}
		else
		{
			++itr;
		}
	}
}

void Network::Graph::BuildHalfedge()
{
	auto pair = [](Edge& e0, Edge& e1) {
		e0.pair = &e1;
		e1.pair = &e0;
	};
	auto conn = [](Edge& e0, Edge& e1) {
		e0.next = &e1;
		e1.prev = &e0;
	};

	for (auto itr = vertices.begin(); itr != vertices.end(); ++itr)
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

}