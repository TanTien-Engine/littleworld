#pragma once

#include <SM_Vector.h>

#include <memory>
#include <vector>

#define CGAL_NO_GMP 1
#undef BOOST_NONCOPYABLE_BASE_TOKEN_DEFINED
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/create_straight_skeleton_2.h>
#include <boost/smart_ptr/shared_ptr.hpp>

namespace citygen
{

class Graph;

class StraightSkeleton
{
public:
	StraightSkeleton(const std::vector<sm::vec2>& border);

	std::vector<std::vector<sm::vec2>> Offset(float distance) const;

	std::vector<std::vector<sm::vec2>> Faces() const;

	std::vector<sm::vec2> TravelStroke(const std::vector<sm::vec2>& border) const;

	std::map<sm::vec2, float> GetAllNodeDist() const;

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

private:
	void BuildPathTree() const;

private:
	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef CGAL::Straight_skeleton_2<K> Skeleton;
	typedef boost::shared_ptr<Skeleton> SkeletonPtr;
	typedef CGAL::Polygon_2<K>           Polygon_2;
	typedef boost::shared_ptr<Polygon_2> PolygonPtr;

	static std::vector<std::vector<sm::vec2>> GetBorder(const std::vector<PolygonPtr>& polys);

	mutable SkeletonPtr m_in_skel = nullptr;
	mutable SkeletonPtr m_ex_skel = nullptr;

private:
	PolygonPtr m_poly;

	mutable std::shared_ptr<Path> m_path_tree_root = nullptr;

}; // StraightSkeleton

}