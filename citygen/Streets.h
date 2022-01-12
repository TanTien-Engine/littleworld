#pragma once

#include <SM_Vector.h>

#include <vector>
#include <set>
#include <memory>

namespace citygen
{

class TensorField;

class Streets
{
public:
	Streets(const std::shared_ptr<TensorField>& tf);

	void BuildStreamlines(int num);
	void BuildTopology();

	auto& GetMajorPaths() const { return m_major_paths; }
	auto& GetMinorPaths() const { return m_minor_paths; }

	std::vector<sm::vec2> GetNodes() const;
	std::vector<std::vector<sm::vec2>> GetPolygons() const;

private:
	struct Vertex;
	struct Edge;

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

	struct Graph
	{
		~Graph();

		Vertex* AddVertex(const sm::vec2& pos);
		void AddPath(const std::vector<sm::vec2>& path);

		void RemoveDegTwoVert();

		void BuildHalfedge();

		std::set<Vertex*, VertexComp> vertices;
	};

private:
	std::vector<sm::vec2> BuildPath(const sm::ivec2& p, bool major) const;

	std::vector<sm::vec2> Travel(const sm::ivec2& p, bool major, bool forward) const;
	sm::vec2 CalcDir(const sm::ivec2& p, bool major) const;

	void IntersectPaths();
	static void IntersectPaths(std::vector<sm::vec2>& p0, std::vector<sm::vec2>& p1);
	static std::vector<sm::vec2> PathCutBy(const std::vector<sm::vec2>& base, const std::vector<sm::vec2>& cut);

	void BuildGraph();

private:
	std::shared_ptr<TensorField> m_tf = nullptr;

	std::vector<sm::vec2> m_border;
	std::vector<std::vector<sm::vec2>> m_major_paths, m_minor_paths;

	std::shared_ptr<Graph> m_graph = nullptr;

}; // Streets

}