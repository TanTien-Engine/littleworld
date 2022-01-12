#pragma once

#include <SM_Vector.h>

#include <vector>
#include <set>

namespace citygen
{

class Graph
{
public:
	~Graph();

	void AddPath(const std::vector<sm::vec2>& path);

	void RemoveDegTwoVert();

	void BuildHalfedge();

	auto& GetVertices() const { return m_vertices; }
	std::vector<std::vector<sm::vec2>> GetPolygons() const;

private:
	struct Vertex;
	struct Edge;

	Vertex* AddVertex(const sm::vec2& pos);

	struct VertexComp
	{
		bool operator () (const Vertex* lhs, const Vertex* rhs) const;
	};

	struct EdgeComp
	{
		bool operator () (const Edge& lhs, const Edge& rhs) const;
	};

	struct Vertex
	{
		Vertex(const sm::vec2& pos) : pos(pos) {}

		sm::vec2 pos;

		std::set<Edge, EdgeComp> edges;
	};

	struct Edge
	{
		Edge(Vertex* f, Vertex* t) : f(f), t(t) {
			assert(f != t);
		}

		Vertex* f = nullptr;
		Vertex* t = nullptr;

		const Edge* pair = nullptr;
		const Edge* prev = nullptr;
		const Edge* next = nullptr;

		mutable bool visited = false;
	};

private:
	std::set<Vertex*, VertexComp> m_vertices;

}; // Graph

}