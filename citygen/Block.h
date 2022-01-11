#pragma once

#include <halfedge/Polygon.h>
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

class Block
{
public:
	Block(const std::vector<sm::vec2>& border);

	std::vector<std::vector<sm::vec2>> Offset(float distance) const;

	std::vector<std::vector<sm::vec2>> GetBorder() const;

private:
	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef CGAL::Polygon_2<K>           Polygon_2;
	typedef CGAL::Straight_skeleton_2<K> Skeleton;
	typedef boost::shared_ptr<Polygon_2> PolygonPtr;
	typedef boost::shared_ptr<Skeleton>  SkeletonPtr;

	struct Polygon
	{
		Polygon(const PolygonPtr& poly) : poly(poly) {}

		PolygonPtr  poly = nullptr;
		mutable SkeletonPtr in_skel = nullptr;
		mutable SkeletonPtr ex_skel = nullptr;
	};

	static std::vector<std::vector<sm::vec2>> GetBorder(const std::vector<PolygonPtr>& polys);

private:
	std::vector<Polygon> m_polys;

}; // Block

}