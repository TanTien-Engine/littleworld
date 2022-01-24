#include "Streets.h"
#include "TensorField.h"
#include "Graph.h"
#include "KMeans.h"
#include "RotatingCalipers.h"
#include "Math.h"

#include <sm/SM_DouglasPeucker.h>
#include <SM_Calc.h>

#include <iterator>
#include <map>

//#define BILINEAR_SAMPLING
//#define TRAVEL_STOP_CHECK

namespace citygen
{

Streets::Streets(const std::shared_ptr<TensorField>& tf,
	             const std::vector<sm::vec2>& border)
	: m_tf(tf)
	, m_border(border)
{
}

void Streets::BuildStreamlines(int num, bool random)
{
	auto w = m_tf->GetWidth();
	auto h = m_tf->GetHeight();

	sm::rect aabb;
	for (auto& p : m_border) {
		aabb.Combine(p);
	}

	sm::rect region(aabb);
	region.xmin *= w;
	region.xmax *= w;
	region.ymin *= h;
	region.ymax *= h;

	if (random)
	{
		std::vector<std::shared_ptr<Path>> major_paths, minor_paths;

		srand(static_cast<unsigned int>(m_seed * UINT32_MAX));
		for (int i = 0, n = aabb.Width() * aabb.Height() * 100; i < n; ++i)
		{
			float x = aabb.xmin + static_cast<float>(rand()) / RAND_MAX * aabb.Width();
			float y = aabb.ymin + static_cast<float>(rand()) / RAND_MAX * aabb.Height();
			if (sm::is_point_in_area(sm::vec2(x, y), m_border))
			{
				int ix = static_cast<int>(w * x);
				int iy = static_cast<int>(h * y);

				auto major_pts = BuildPath(sm::ivec2(ix, iy), region, true);
				if (!major_pts.empty()) {
					major_paths.push_back(std::make_shared<Path>(major_pts));
				}

				auto minor_pts = BuildPath(sm::ivec2(ix, iy), region, false);
				if (!minor_pts.empty()) {
					minor_paths.push_back(std::make_shared<Path>(minor_pts));
				}
			}
		}

		if (major_paths.size() > num)
		{
			m_major_paths = SelectPaths(major_paths, num, aabb);
			m_minor_paths = SelectPaths(minor_paths, num, aabb);
		}
		else
		{
			m_major_paths = major_paths;
			m_minor_paths = minor_paths;
		}
	}
	else
	{
		for (size_t i = 0; i < num; ++i)
		{
			int x = static_cast<int>(w / num * (0.5f + i));
			int y = static_cast<int>(h / num * (0.5f + i));
	
			auto major_pts = BuildPath(sm::ivec2(x, y), region, true);
			if (!major_pts.empty()) {
				m_major_paths.push_back(std::make_shared<Path>(major_pts));
			}

			auto minor_pts = BuildPath(sm::ivec2(x, y), region, false);
			if (!minor_pts.empty()) {
				m_minor_paths.push_back(std::make_shared<Path>(minor_pts));
			}
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
	std::vector<std::vector<sm::vec2>> polys;
	for (auto& poly : m_graph->GetPolygons()) {
		if (!sm::is_polygon_clockwise(poly)) {
			polys.push_back(poly);
		}
	}
	return polys;
}

bool Streets::TranslateNodes(const std::vector<sm::vec2>& nodes)
{
	std::vector<Graph::Vertex*> ori_nodes;
	for (auto& vert : m_graph->GetVertices()) {
		if (vert->edges.size() != 2) {
			ori_nodes.push_back(vert);
		}
	}

	if (nodes.size() != ori_nodes.size()) {
		return false;
	}

	bool dirty = false;
	for (int i = 0, n = nodes.size(); i < n; ++i) {
		if (TranslateNode(ori_nodes[i], nodes[i])) {
			dirty = true;
		}
	}

	return dirty;
}

std::vector<sm::vec2> Streets::BuildPath(const sm::ivec2& p, const sm::rect& region, bool major) const
{
	std::vector<sm::vec2> points;

	bool is_loop = false;
	auto f = Travel(p, region, major, true, &is_loop);
	if (is_loop) 
	{
		points = f;
	}
	else
	{
		points = Travel(p, region, major, false, &is_loop);
		std::reverse(points.begin(), points.end());
		std::copy(f.begin(), f.end(), std::back_inserter(points));
	}

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

std::vector<sm::vec2> Streets::Travel(const sm::ivec2& p, const sm::rect& region, bool major, bool forward, bool* is_loop) const
{
	std::vector<sm::vec2> points;

	sm::vec2 first((float)p.x, (float)p.y);
	sm::vec2 fp(first);
	sm::vec2 prev_dir = sm::vec2(0, 0);
	

#ifdef TRAVEL_STOP_CHECK
	const int scale = 2;
	std::vector<bool> visited(m_tf->GetWidth() / scale * m_tf->GetHeight() / scale, false);
#endif // TRAVEL_STOP_CHECK

	float len = 0.0f;

	const int max_steps = m_tf->GetWidth() * 2;
	int prev_idx = 0;
	for (size_t i = 0; i < max_steps; ++i)
	{
		if (!sm::is_point_in_rect(fp, region)) {
			points.push_back(fp);
			break;
		}

		if (i > 100 && sm::dis_pos_to_pos(fp, first) < 20.0f)
		{
			points.push_back(first);
			*is_loop = true;
			break;
		}

		points.push_back(fp);

#ifdef TRAVEL_STOP_CHECK
		if (fp.x < 0 || fp.x > m_tf->GetWidth() ||
			fp.y < 0 || fp.y > m_tf->GetHeight()) {
			break;
		}

		const int idx = ((int)fp.y / scale) * (m_tf->GetWidth() / scale) + ((int)fp.x / scale);
		if (idx != prev_idx && visited[idx]) {
			break;
		}

		visited[idx] = true;
		//if (i == 0) 
		//{
		//	const int cx = (int)fp.x;
		//	const int cy = (int)fp.y;
		//	const int r = 1;
		//	for (int y = cy - r; y < cy + r; ++y) {
		//		for (int x = cx - r; x < cx + r; ++x) {
		//			if (x >= 0 && y >= 0 && x < m_tf->GetWidth() && y < m_tf->GetHeight()) {
		//				const int idx = (y / scale) * (m_tf->GetWidth() / scale) + (x / scale);
		//				visited[idx] = true;
		//			}
		//		}
		//	}
		//}
		prev_idx = idx;
#endif // TRAVEL_STOP_CHECK

		auto dir = CalcDir(fp, major);
		if (!forward) {
			dir = -dir;
		}
		if (prev_dir != sm::vec2(0, 0) && prev_dir.Dot(dir) < 0) {
			dir = -dir;
		}
		prev_dir = dir;

		if (dir.x == 0 && dir.y == 0) {
			break;
		}
		fp += dir;
	}

	if (points.size() == 1) {
		points.clear();
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

#ifdef BILINEAR_SAMPLING
	if (x_max > m_tf->GetWidth() || y_max > m_tf->GetHeight()) 
	{
		tensor = m_tf->GetTensor(x_min, y_min);
	}
	else
	{
		sm::vec4 t_down = m_tf->GetTensor(x_min, y_min) * (x_max - p.x) + m_tf->GetTensor(x_max, y_min) * (p.x - x_min);
		sm::vec4 t_up   = m_tf->GetTensor(x_min, y_max) * (x_max - p.x) + m_tf->GetTensor(x_max, y_max) * (p.x - x_min);
		tensor = t_down * (y_max - p.y) + t_up * (p.y - y_min);
	}
#else
	tensor = m_tf->GetTensor(x_min, y_min);
#endif // BILINEAR_SAMPLING

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
	for (auto& p : m_major_paths) {
		Math::IntersectPaths(m_border, p->m_pts);
	}
	for (auto& p : m_minor_paths) {
		Math::IntersectPaths(m_border, p->m_pts);
	}

	for (auto& p : m_major_paths) {
		p->Trim(m_border);
	}
	for (auto& p : m_minor_paths) {
		p->Trim(m_border);
	}

	for (auto& p0 : m_major_paths) {
		for (auto& p1 : m_minor_paths) {
			Math::IntersectPaths(p0->m_pts, p1->m_pts);
		}
	}
}

void Streets::BuildGraph()
{
	m_graph = std::make_shared<Graph>();

	m_graph->AddPath(m_border);
	for (auto& path : m_major_paths) {
		m_graph->AddPath(path->GetPoints());
	}
	for (auto& path : m_minor_paths) {
		m_graph->AddPath(path->GetPoints());
	}

	//std::vector<int> num(10, 0);
	//for (auto& vert : m_graph->GetVertices()) {
	//	++num[vert->edges.size()];
	//}

	//m_graph->RemoveDegTwoVert();

//	m_graph->VertexMerge(0.005f);

	m_graph->BuildHalfedge();

	BindGraph2Path();
}

void Streets::BindGraph2Path()
{
	auto bind_path = [&](const std::shared_ptr<Path>& path)
	{
		path->m_verts.clear();

		for (auto& p : path->m_pts)
		{
			auto v = m_graph->QueryVertex(p);
			assert(v);
			path->m_verts.push_back(v);
		}
	};

	for (auto& path : m_major_paths) {
		bind_path(path);
	}
	for (auto& path : m_minor_paths) {
		bind_path(path);
	}
}

std::vector<std::shared_ptr<Streets::Path>>
Streets::SelectPaths(const std::vector<std::shared_ptr<Path>>& paths, int num, const sm::rect& aabb) const
{
	std::vector<sm::vec2> pts;
	for (auto& p : paths) {
		pts.push_back(p->m_center);
	}
	KMeans kmeans(pts);
	auto groups = kmeans.Clustering(num * 2, 0.005f);

	PathComp cmp(aabb);

	std::vector<std::shared_ptr<Streets::Path>> ret;

	for (auto& g : groups)
	{
		if (g.empty()) {
			continue;
		}

		std::vector<std::shared_ptr<Streets::Path>> tmp;
		for (auto i : g) {
			tmp.push_back(paths[i]);
		}
		std::sort(tmp.begin(), tmp.end(), cmp);

		ret.push_back(tmp.front());
	}

	std::sort(ret.begin(), ret.end(), cmp);
	ret.erase(ret.begin() + num, ret.end());

	return ret;
}

bool Streets::TranslateNode(Graph::Vertex* v, const sm::vec2& p)
{
	if (v->pos == p || sm::dis_pos_to_pos(v->pos, p) < m_graph->GetEpsilon()) {
		return false;
	}

	auto offset = p - v->pos;

	for (auto& e : v->edges)
	{
		std::vector<Graph::Vertex*> path;

		auto curr = e;
		while (true)
		{
			path.push_back(curr->t);
			if (curr->t->edges.size() == 2)
			{
				curr = curr->t->edges[0]->t == curr->f ? curr->t->edges[1] : curr->t->edges[0];
			}
			else
			{
				break;
			}
		}

		if (path.empty()) {
			continue;
		}

		float tot_d = sm::dis_pos_to_pos(v->pos, path[0]->pos);
		for (int i = 0, n = path.size() - 1; i < n; ++i) {
			tot_d += sm::dis_pos_to_pos(path[i]->pos, path[i + 1]->pos);
		}

		for (int i = 0, n = path.size() - 1; i < n; ++i)
		{
			auto d = sm::dis_pos_to_pos(v->pos, path[i]->pos);
			assert(d < tot_d);
			path[i]->pos += offset / tot_d * (tot_d - d);
		}
	}

	v->pos = p;

	return true;
}

//////////////////////////////////////////////////////////////////////////
// class Streets::Path
//////////////////////////////////////////////////////////////////////////

Streets::Path::Path(const std::vector<sm::vec2>& pts)
	: m_pts(pts)
{
	Build();
}

void Streets::Path::Trim(const std::vector<sm::vec2>& border)
{
	std::vector<std::shared_ptr<Path>> paths;

	std::vector<sm::vec2> pts;
	for (auto& p : m_pts)
	{
		bool on_border = false;
		for (auto& b : border) {
			if (sm::dis_pos_to_pos(p, b) < SM_LARGE_EPSILON) {
				on_border = true;
				break;
			}
		}

		if (on_border || sm::is_point_in_area(p, border)) 
		{
			pts.push_back(p);
		} 
		else 
		{
			if (pts.size() > 1) {
				paths.push_back(std::make_shared<Path>(pts));
			}
			pts.clear();
		}
	}
	if (pts.size() > 1) {
		paths.push_back(std::make_shared<Path>(pts));
	}

	if (paths.empty()) {
		return;
	}

	std::sort(paths.begin(), paths.end(), PathComp());

	m_pts = paths.front()->m_pts;

	Build();
}

std::vector<sm::vec2> Streets::Path::GetGraphPoints() const
{
	std::vector<sm::vec2> pts;
	for (auto& v : m_verts) {
		pts.push_back(v->pos);
	}
	return pts;
}

void Streets::Path::Build()
{
	if (m_pts.empty()) {
		return;
	}

	m_length = 0;
	for (size_t i = 0, n = m_pts.size() - 1; i < n; ++i) {
		m_length += sm::dis_pos_to_pos(m_pts[i], m_pts[i + 1]);
	}

	m_loop = m_pts.size() > 1 && m_pts.front() == m_pts.back();

	auto obb = RotatingCalipers::CalcOBB(m_pts, m_loop);

	m_center = sm::rotate_vector(obb.first.Center(), obb.second);

	auto& aabb = obb.first;
	m_area = aabb.Width() * aabb.Height();
	m_perimeter = aabb.Width() + aabb.Height();
}

//////////////////////////////////////////////////////////////////////////
// class Streets::PathComp
//////////////////////////////////////////////////////////////////////////

bool Streets::PathComp::operator () (const std::shared_ptr<Path>& lhs, const std::shared_ptr<Path>& rhs) const
{
	return CalcPathVal(lhs) > CalcPathVal(rhs);
}

float Streets::PathComp::CalcPathVal(const std::shared_ptr<Path>& path) const
{
//	float v = path->m_length + path->m_area * 5 + path->m_perimeter;

	float v = path->m_length + path->m_perimeter;

	// center
	if (aabb.IsValid() && aabb.Width() < 1.0f)
	{
		auto c = aabb.Center();
		float d = fabs(path->m_center.x - c.x) / aabb.Width()
			    + fabs(path->m_center.y - c.y) / aabb.Height();
		v -= d * 0.1f;
	}

	if (path->m_loop) {
		v += 10.0f;
	}

	return v;
}

}