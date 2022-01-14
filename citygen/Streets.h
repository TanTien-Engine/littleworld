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

	void SetSeed(float seed) { m_seed = seed; }

public:
	struct PathComp;

	class Path
	{
	public:
		Path(const std::vector<sm::vec2>& pts);

		auto& GetPoints() const { return m_pts; }

	private:
		void Init();

	private:
		std::vector<sm::vec2> m_pts;

		float m_length = 0.0f;
		float m_area = 0.0f;
		float m_perimeter = 0.0f;

		sm::vec2 m_center;

		bool m_loop = false;

		friend struct PathComp;
		friend class Streets;

	}; // Path

	struct PathComp
	{
		bool operator () (const std::shared_ptr<Path>& lhs, const std::shared_ptr<Path>& rhs) const;

		float CalcPathVal(const std::shared_ptr<Path>& path) const;
	};

private:
	std::vector<sm::vec2> BuildPath(const sm::ivec2& p, bool major) const;

	std::vector<sm::vec2> Travel(const sm::ivec2& p, bool major, bool forward, bool* is_loop) const;
	sm::vec2 CalcDir(const sm::vec2& p, bool major) const;

	void IntersectPaths();
	static void IntersectPaths(std::vector<sm::vec2>& p0, std::vector<sm::vec2>& p1);
	static std::vector<sm::vec2> PathCutBy(const std::vector<sm::vec2>& base, const std::vector<sm::vec2>& cut);

	void BuildGraph();

	std::vector<std::shared_ptr<Path>> SelectPaths(const std::vector<std::shared_ptr<Path>>& paths, int num) const;

private:
	std::shared_ptr<TensorField> m_tf = nullptr;

	std::vector<sm::vec2> m_border;
	std::vector<std::shared_ptr<Path>> m_major_paths, m_minor_paths;

	std::shared_ptr<Graph> m_graph = nullptr;

	float m_seed = 0.0f;

}; // Streets

}