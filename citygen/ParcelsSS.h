#pragma once

#include <SM_Vector.h>

#define CGAL_NO_GMP 1
#undef BOOST_NONCOPYABLE_BASE_TOKEN_DEFINED
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/create_straight_skeleton_2.h>
#include <boost/smart_ptr/shared_ptr.hpp>

#include <vector>
#include <memory>

namespace citygen
{

class Graph;

class ParcelsSS
{
public:
	ParcelsSS(const std::vector<sm::vec2>& border);

	void Build(float max_len);

	std::vector<std::vector<sm::vec2>> GetPolygons() const;

	void SetSeed(float seed) { m_seed = seed; }

private:
	struct Node;

	struct Path
	{
		std::shared_ptr<Node> nodes[2];

		std::vector<sm::vec2> pts;
	};

	struct Node
	{
		std::vector<std::shared_ptr<Path>> paths;

		sm::vec2 pos;

		float dist = FLT_MAX;
	};

	void BuildGraph();

	std::vector<sm::vec2> TravelSkeleton(float& border_width) const;

private:
	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef CGAL::Polygon_2<K>           Polygon_2;
	typedef CGAL::Straight_skeleton_2<K> Skeleton;
	typedef boost::shared_ptr<Polygon_2> PolygonPtr;
	typedef boost::shared_ptr<Skeleton>  SkeletonPtr;

	std::vector<sm::vec2> m_border;

	std::shared_ptr<Graph> m_graph = nullptr;

	PolygonPtr  m_poly = nullptr;
	SkeletonPtr m_skel = nullptr;

	std::shared_ptr<Path> m_root = nullptr;

	float m_seed = 0.0f;
	float m_max_len = 0.1f;

}; // ParcelsSS

}