#pragma once

#include <SM_Vector.h>
#include <sm_const.h>

#include <vector>
#include <set>

namespace citygen
{

class Graph
{
public:
	Graph(float epsilon = SM_LARGE_EPSILON * 10);
	~Graph();

	void AddPath(const std::vector<sm::vec2>& path);

	void RemoveDegTwoVert();

	void BuildHalfedge();

	auto& GetVertices() const { return m_vertices; }
	std::vector<std::vector<sm::vec2>> GetPolygons() const;

	void VertexMerge(float dist);

	float GetEpsilon() const { return m_epsilon; }

public:
	struct Edge;

	struct Vertex
	{
		Vertex(const sm::vec2& pos) : pos(pos) {}
		~Vertex();

		void AddEdge(Vertex* vert);

		sm::vec2 pos;

		std::vector<Edge*> edges;
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

	Vertex* QueryVertex(const sm::vec2& pos) const;

private:
	Vertex* AddVertex(const sm::vec2& pos);

	struct VertexComp
	{
		bool operator () (const Vertex* lhs, const Vertex* rhs) const;
	};

	struct EdgeComp
	{
		bool operator () (const Edge* lhs, const Edge* rhs) const;
	};

private:
	float m_epsilon;

	std::set<Vertex*, VertexComp> m_vertices;

}; // Graph

}