#include "Parcels.h"
#include "RotatingCalipers.h"

#include <SM_Calc.h>
#include <SM_Polyline.h>

#include <queue>
#include <iterator>

namespace citygen
{

Parcels::Parcels(const std::vector<sm::vec2>& border)
	: m_border(border)
{
}

void Parcels::Build(float max_len)
{
	srand(static_cast<unsigned int>(m_seed * UINT32_MAX));

	m_root = std::make_shared<Node>(m_border, max_len);
}

void Parcels::Offset(float distance)
{
	std::queue<std::shared_ptr<Node>> buf;
	buf.push(m_root);

	while (!buf.empty())
	{
		auto n = buf.front(); buf.pop();

		if (!n->children[0] && !n->children[1])
		{
			auto offset_poly = sm::polyline_offset(n->poly, distance, true);
			if (!offset_poly.empty()) {
				n->poly_offset = offset_poly.front();
			} else {
				n->poly_offset.clear();
			}
		}

		for (auto& c : n->children) {
			if (c) {
				buf.push(c);
			}
		}
	}
}

std::vector<std::vector<sm::vec2>> Parcels::GetPolygons() const
{
	std::vector<std::vector<sm::vec2>> polygons;

	std::queue<std::shared_ptr<Node>> buf;
	buf.push(m_root);

	while (!buf.empty())
	{
		auto n = buf.front(); buf.pop();

		if (!n->children[0] && !n->children[1] && !n->poly_offset.empty()) {
			polygons.push_back(n->poly_offset);
		}

		for (auto& c : n->children) {
			if (c) {
				buf.push(c);
			}
		}
	}

	return polygons;
}

Parcels::Node::Node(const std::vector<sm::vec2>& poly, float max_len)
	: poly(poly)
{
	obb = RotatingCalipers::CalcOBB(poly, true);

	const float angle = obb.second;
	const float d_ang = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.2f;
	const float new_ang = angle + d_ang;

	const float min_len = std::min(obb.first.Width(), obb.first.Height());
	const float d_trans = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.3f * min_len;

	auto sz = obb.first.Size();
	if (sz.x > max_len && sz.x >= sz.y)
	{
		auto c = sm::rotate_vector(obb.first.Center(), angle);
		auto p0 = sm::rotate_vector(sm::vec2(d_trans, -10), new_ang) + c;
		auto p1 = sm::rotate_vector(sm::vec2(d_trans, 10), new_ang) + c;
		Clip(p0, p1, max_len);
	}
	else if (sz.y > max_len && sz.y >= sz.x)
	{
		auto c = sm::rotate_vector(obb.first.Center(), angle);
		auto p0 = sm::rotate_vector(sm::vec2(-10, d_trans), new_ang) + c;
		auto p1 = sm::rotate_vector(sm::vec2(10, d_trans), new_ang) + c;
		Clip(p0, p1, max_len);
	}
}

void Parcels::Node::Clip(const sm::vec2& p0, const sm::vec2& p1, float max_len)
{
	int find = 0;
	std::vector<sm::vec2> poly0, poly1;

	auto do_cross = [&](const sm::vec2& cross)
	{
		if (find == 0)
		{
			poly0.push_back(cross);
			poly1.push_back(cross);
			find = 1;
		}
		else if (find == 1)
		{
			poly0.push_back(cross);
			poly1.push_back(cross);
			find = 2;
		}
	};

	for (size_t i = 0, n = poly.size(); i < n; ++i)
	{
		sm::vec2 cross;
		cross.MakeInvalid();

		if (sm::is_point_in_segment(poly[i], p0, p1)) {
			cross = poly[i];
		} 

		if (cross.IsValid()) {
			do_cross(cross);
		} else {
			if (find == 0) {
				poly0.push_back(poly[i]);
			} else if (find == 1) {
				poly1.push_back(poly[i]);
			} else {
				poly0.push_back(poly[i]);
			}
		}

		if (sm::intersect_segment_segment(poly[i], poly[(i + 1) % n], p0, p1, &cross)) {
			do_cross(cross);
		}
	}

	if (poly0.size() > 2 && poly1.size() > 2)
	{
		children[0] = std::make_shared<Node>(poly0, max_len);
		children[1] = std::make_shared<Node>(poly1, max_len);
	}
}

}