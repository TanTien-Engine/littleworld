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

	void Build();

	std::vector<std::vector<sm::vec2>> GetPolygons(float dist) const;

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

//	std::shared_ptr<Path> m_root = nullptr;
	std::vector<std::shared_ptr<Path>> m_paths;

}; // ParcelsSS

}