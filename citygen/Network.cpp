#include "Network.h"
#include "TensorField.h"

#include <sm/SM_DouglasPeucker.h>

#include <iterator>

namespace citygen
{

Network::Network(const std::shared_ptr<TensorField>& tf)
	: m_tf(tf)
{
}

void Network::BuildStreamlines(int num)
{
	auto w = m_tf->GetWidth();
	auto h = m_tf->GetHeight();
	
	for (int iy = 0; iy < num; ++iy) 
	{
		int y = h / num * (0.5 + iy);
		for (int ix = 0; ix < num; ++ix) 
		{
			int x = w / num * (0.5 + ix);
			
			m_major_paths.push_back(BuildPath(sm::ivec2(x, y), true));
			m_minor_paths.push_back(BuildPath(sm::ivec2(x, y), false));
		}
	}
}

void Network::BuildTopology()
{

}

std::shared_ptr<Network::Path> 
Network::BuildPath(const sm::ivec2& p, bool major) const
{
	auto f = Travel(p, major, true);
	auto b = Travel(p, major, false);

	auto points = b;
	std::reverse(points.begin(), points.end());
	std::copy(f.begin(), f.end(), std::back_inserter(points));

	auto path = std::make_shared<Path>();
	sm::douglas_peucker(points, 0.5f, path->points);

	// to 0-1
	auto w = m_tf->GetWidth();
	auto h = m_tf->GetHeight();
	for (auto& p : path->points) {
		p.x /= w;
		p.y /= h;
	}

	return path;
}

std::vector<sm::vec2> Network::Travel(const sm::ivec2& p, bool major, bool forward) const
{
	std::vector<sm::vec2> points;

	sm::vec2 fp(p.x, p.y);
	sm::vec2 prev_dir = sm::vec2(0, 0);

	const int max_steps = m_tf->GetWidth() * 2;
	for (int i = 0; i < max_steps; ++i)
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

}