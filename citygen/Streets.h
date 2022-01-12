#pragma once

#include <SM_Vector.h>

#include <memory>
#include <vector>

namespace citygen
{

class TensorField;
class Graph;

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
	std::vector<sm::vec2> BuildPath(const sm::ivec2& p, bool major) const;

	std::vector<sm::vec2> Travel(const sm::ivec2& p, bool major, bool forward) const;
	sm::vec2 CalcDir(const sm::vec2& p, bool major) const;

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