#include "Extrude.h"

#include <geoshape/Polygon2D.h>
#include <polymesh3/Polytope.h>

#define CGAL_NO_GMP 1
#undef BOOST_NONCOPYABLE_BASE_TOKEN_DEFINED
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/create_straight_skeleton_2.h>
#include <boost/smart_ptr/shared_ptr.hpp>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Polygon_2<K>           Polygon_2;
typedef CGAL::Straight_skeleton_2<K> Skeleton;
typedef boost::shared_ptr<Polygon_2> PolygonPtr;
typedef boost::shared_ptr<Skeleton>  SkeletonPtr;

namespace
{

class PolyBuilder
{
public:
	PolyBuilder() {}

    void AddFace(const std::vector<sm::vec2>& border, const std::vector<std::vector<sm::vec2>>& holes, float height = 0.0f)
    {
        if (border.empty()) {
            return;
        }

        auto face = std::make_shared<pm3::Polytope::Face>();

        face->border = AddPoints(border, height);
        for (auto& hole : holes) {
            face->holes.push_back(AddPoints(hole, height));
        }

        m_faces.push_back(face);
    }

    std::shared_ptr<pm3::Polytope> CreatePolytope() const
    {
        return std::make_shared<pm3::Polytope>(m_verts, m_faces);
    }

    auto& GetVertices() const { return m_verts; }

private:
    std::vector<size_t> AddPoints(const std::vector<sm::vec2>& points, float height = 0.0f)
    {
        std::vector<size_t> idxes;

        for (auto& p : points)
        {
            size_t idx = 0;

            auto itr = m_pos2idx.find(p);
            if (itr != m_pos2idx.end()) 
            {
                idx = itr->second;
            } 
            else 
            {
                auto vert = std::make_shared<pm3::Polytope::Point>(sm::vec3(p.x, height, p.y));
                idx = m_verts.size();
                m_pos2idx.insert({ p, idx });
                m_verts.push_back(vert);
            }

            idxes.push_back(idx);
        }

        return idxes;
    }

private:
    std::vector<pm3::Polytope::PointPtr> m_verts;
    std::vector<pm3::Polytope::FacePtr>  m_faces;

	std::map<sm::vec2, size_t> m_pos2idx;

}; // PolyBuilder

}

namespace citygen
{

std::shared_ptr<pm3::Polytope> 
Extrude::Face(const std::shared_ptr<gs::Polygon2D>& polygon, float distance)
{
    PolyBuilder builder;
    builder.AddFace(polygon->GetVertices(), polygon->GetHoles());
    auto polytope = builder.CreatePolytope();
    auto topo_poly = polytope->GetTopoPoly();

    bool create_face[he::Polyhedron::ExtrudeMaxCount];
    create_face[he::Polyhedron::ExtrudeFront] = true;
    create_face[he::Polyhedron::ExtrudeBack] = true;
    create_face[he::Polyhedron::ExtrudeSide] = true;

    std::vector<he::TopoID> face_ids;
    auto& faces = topo_poly->GetLoops();
    face_ids.reserve(faces.Size());
    auto first_f = faces.Head();
    auto curr_f = first_f;
    do {
        face_ids.push_back(curr_f->ids);
        curr_f = curr_f->linked_next;
    } while (curr_f != first_f);

    if (topo_poly->Extrude(-distance, face_ids, create_face)) {
        polytope->BuildFromTopo();
    }

    return polytope;
}

std::shared_ptr<pm3::Polytope> 
Extrude::Skeleton(const std::shared_ptr<gs::Polygon2D>& polygon, float distance)
{
    PolyBuilder builder;
    builder.AddFace(polygon->GetVertices(), polygon->GetHoles());

    auto poly = PolygonPtr(new Polygon_2);
    for (auto& p : polygon->GetVertices()) {
        poly->push_back(K::Point_2(p.x, p.y));
    }

    auto skel = CGAL::create_interior_straight_skeleton_2(poly);
    if (!skel) {
        return Face(polygon, distance);
    }
    for (auto itr = skel->faces_begin(); itr != skel->faces_end(); ++itr)
    {
        std::vector<sm::vec2> pts;

        auto first = itr->halfedge();
        auto curr = first;
        do {
            auto p = curr->vertex()->point();
            pts.push_back(sm::vec2((float)p.x(), (float)p.y()));

            curr = curr->next();
        } while (curr != first);

        builder.AddFace(pts, {}, distance);
    }

    std::set<sm::vec2> border_set;
    for (auto& p : polygon->GetVertices()) {
        border_set.insert(p);
    }

    auto polytope = builder.CreatePolytope();

    polytope->GetTopoPoly();
    polytope->BuildFromTopo();

    return polytope;
}

}