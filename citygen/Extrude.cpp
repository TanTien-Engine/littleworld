#include "Extrude.h"
#include "StraightSkeleton.h"
#include "PolyBuilder.h"
#include "../archgen/RoofSkeleton.h"

#include <geoshape/Polygon2D.h>
#include <polymesh3/Polytope.h>

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
    archgen::RoofSkeleton rs(polygon->GetVertices());

    auto& faces = rs.Faces();
    if (faces.empty()) {
        return Face(polygon, distance);
    }

    PolyBuilder builder;
    builder.AddFace(polygon->GetVertices(), polygon->GetHoles());
    for (auto& f : faces) 
    {
        std::vector<sm::vec3> face_h;
        for (auto& p : f) {
            float dist = rs.QueryDist(p);
            face_h.push_back({ p.x, dist * 0.5f, p.y });
        }
        builder.AddFace(face_h);
    }

    auto polytope = builder.CreatePolytope();

    polytope->GetTopoPoly();
    polytope->BuildFromTopo();

    return polytope;
}

}