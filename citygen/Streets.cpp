#include "Streets.h"
#include "TensorField.h"
#include "Graph.h"

#include <sm/SM_DouglasPeucker.h>
#include <SM_Calc.h>

#include <iterator>
#include <map>

namespace citygen
{

Streets::Streets(const std::shared_ptr<TensorField>& tf)
	: m_tf(tf)
{
}

void Streets::BuildStreamlines(int num)
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

void Streets::BuildTopology()
{
	IntersectPaths();
	BuildGraph();
}

std::vector<sm::vec2> Streets::GetNodes() const
{
	std::vector<sm::vec2> ret;
	for (auto& vert : m_graph->GetVertices()) {
		if (vert->edges.size() != 2) {
			ret.push_back(vert->pos);
		}
	}
	return ret;
}

std::vector<std::vector<sm::vec2>> Streets::GetPolygons() const
{
	auto blocks = m_graph->GetPolygons();

	// sikp the outside block
	if (!blocks.empty()) {
		blocks.erase(blocks.begin());
	}

	return blocks;
}

std::vector<sm::vec2> Streets::BuildPath(const sm::ivec2& p, bool major) const
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

std::vector<sm::vec2> Streets::Travel(const sm::ivec2& p, bool major, bool forward) const
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

sm::vec2 Streets::CalcDir(const sm::vec2& p, bool major) const
{
	if (p.x < 0 || p.x > m_tf->GetWidth() ||
		p.y < 0 || p.y > m_tf->GetHeight()) {
		return sm::vec2(0, 0);
	}

	sm::vec4 tensor;

	int x_min = static_cast<int>(floorf(p.x));
	int x_max = x_min + 1;
	int y_min = static_cast<int>(floorf(p.y));
	int y_max = y_min + 1;

	//if (x_max > m_tf->GetWidth() || y_max > m_tf->GetHeight()) 
	//{
		tensor = m_tf->GetTensor(x_min, y_min);
	//}
	//else
	//{
	//	sm::vec4 t_down = m_tf->GetTensor(x_min, y_min) * (x_max - p.x) + m_tf->GetTensor(x_max, y_min) * (p.x - x_min);
	//	sm::vec4 t_up   = m_tf->GetTensor(x_min, y_max) * (x_max - p.x) + m_tf->GetTensor(x_max, y_max) * (p.x - x_min);
	//	tensor = t_down * (y_max - p.y) + t_up * (p.y - y_min);
	//}

	if (abs(tensor.x) < 0.00001) {
		return sm::vec2(0, 0);
	}

	float theta = atan2f(tensor.y, tensor.x) / 2.0f;
	if (!major) {
		theta += SM_PI * 0.5;
	}
	return sm::vec2(cosf(theta), sinf(theta));
}

void Streets::IntersectPaths()
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

void Streets::IntersectPaths(std::vector<sm::vec2>& p0, std::vector<sm::vec2>& p1)
{
	p0 = PathCutBy(p0, p1);
	p1 = PathCutBy(p1, p0);
}

std::vector<sm::vec2> Streets::PathCutBy(const std::vector<sm::vec2>& base, const std::vector<sm::vec2>& cut)
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

void Streets::BuildGraph()
{
	m_graph = std::make_shared<Graph>();

	m_graph->AddPath(m_border);
	for (auto& path : m_major_paths) {
		m_graph->AddPath(path);
	}
	for (auto& path : m_minor_paths) {
		m_graph->AddPath(path);
	}

	std::vector<int> num(10, 0);
	for (auto& vert : m_graph->GetVertices()) {
		++num[vert->edges.size()];
	}

	//m_graph->RemoveDegTwoVert();
	m_graph->BuildHalfedge();
}

}