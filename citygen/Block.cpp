#include "Block.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/create_offset_polygons_2.h>

namespace citygen
{

Block::Block(const std::vector<sm::vec2>& border)
{
    PolygonPtr poly(new Polygon_2);
    for (auto& p : border) {
        poly->push_back(K::Point_2(p.x, p.y));
    }
    m_polys.push_back(poly);
}

std::vector<std::vector<sm::vec2>> Block::Offset(float distance) const
{
    std::vector<std::vector<sm::vec2>> ret;

    for (auto& poly : m_polys)
    {
        float offset = -distance;
        if (offset < 0)
        {
            double d_offset = -offset;

            if (!poly.in_skel) {
                poly.in_skel = CGAL::create_interior_straight_skeleton_2(poly.poly);
            }

            if (poly.in_skel)
            {
                auto polys = CGAL::create_offset_polygons_2<Polygon_2>(d_offset, *poly.in_skel);
                auto border = GetBorder(polys);
                std::copy(border.begin(), border.end(), std::back_inserter(ret));
            }
        }
        else
        {
            double d_offset = offset;

            if (!poly.ex_skel) {
                poly.ex_skel = CGAL::create_exterior_straight_skeleton_2(d_offset * 2, poly.poly);
            }

            if (poly.ex_skel)
            {
                auto polys = CGAL::create_offset_polygons_2<Polygon_2>(d_offset, *poly.ex_skel);
                auto border = GetBorder(polys);
                std::copy(border.begin(), border.end(), std::back_inserter(ret));
            }
        }
    }

    return ret;
}

std::vector<std::vector<sm::vec2>> 
Block::GetBorder() const
{
    std::vector<PolygonPtr> polys;
    for (auto poly : m_polys) {
        polys.push_back(poly.poly);
    }
    return GetBorder(polys);
}

std::vector<std::vector<sm::vec2>> 
Block::GetBorder(const std::vector<PolygonPtr>& polys)
{
    std::vector<std::vector<sm::vec2>> ret;

    for (auto poly : polys)
    {
        std::vector<sm::vec2> loop;
        loop.reserve(poly->size());
        for (auto itr = poly->vertices_begin(); itr != poly->vertices_end(); ++itr) {
            sm::vec2 p(static_cast<float>(itr->x()), static_cast<float>(itr->y()));
            loop.push_back(p);
        }
        ret.push_back(loop);
    }

    return ret;
}

}