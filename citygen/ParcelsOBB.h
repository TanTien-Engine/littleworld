#pragma once

#include <SM_Vector.h>
#include <SM_Rect.h>

#include <memory>
#include <vector>

namespace citygen
{

class ParcelsOBB
{
public:
	ParcelsOBB(const std::vector<sm::vec2>& border);

	void Build(float max_len);

	std::vector<std::vector<sm::vec2>> GetPolygons() const;

	void SetSeed(float seed) { m_seed = seed; }

private:
	struct Node
	{
		Node(const std::vector<sm::vec2>& poly, float max_len);

		void Clip(const sm::vec2& p0, const sm::vec2& p1, float max_len);

		std::vector<sm::vec2> poly;

		std::pair<sm::rect, float> obb;

		std::shared_ptr<Node> children[2] = { nullptr, nullptr };
	};

private:
	std::vector<sm::vec2> m_border;

	std::shared_ptr<Node> m_root;

	float m_seed = 0.0f;

}; // ParcelsOBB

}