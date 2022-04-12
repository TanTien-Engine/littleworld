#include "RoofExtrude.h"
#include "RoofSkeleton.h"
#include "../citygen/PolyBuilder.h"

#include <polymesh3/Polytope.h>
#include <SM_Calc.h>

namespace
{

bool flatting_face(const std::shared_ptr<pm3::Polytope>& poly, const pm3::Polytope::FacePtr& face,
                   std::vector<sm::vec2>& out_poly, float& out_y, sm::mat4& out_mat)
{
    sm::vec3 normal;
    if (!poly->CalcFaceNormal(*face, normal)) {
        return false;
    }
    
    auto rot = sm::mat4(sm::Quaternion::CreateFromVectors(normal, sm::vec3(0, 1, 0)));
    out_mat = rot.Inverted();

    auto& pts = poly->Points();

    std::vector<sm::vec3> rot_face;
    rot_face.reserve(face->border.size());
    for (auto idx : face->border) {
        rot_face.push_back(rot * pts[idx]->pos);
    }

    float y = 0.0f;
    for (auto& p : rot_face) {
        y += p.y;
    }
    y /= rot_face.size();

    out_y = y;

    out_poly.clear();
    out_poly.reserve(rot_face.size());
    for (auto& p : rot_face) {
        out_poly.push_back(sm::vec2(p.x, p.z));
    }
    std::reverse(out_poly.begin(), out_poly.end());

    return true;
}

}

namespace archgen
{

std::shared_ptr<pm3::Polytope> RoofExtrude::
Hip(const std::shared_ptr<pm3::Polytope>& poly, float distance)
{
    if (poly->Faces().size() != 1) {
        return nullptr;
    }

    auto face = poly->Faces().front();

    std::vector<sm::vec2> border;
    float y;
    sm::mat4 mat;
    if (!flatting_face(poly, face, border, y, mat)) {
        return nullptr;
    }

    RoofSkeleton rs(border);

    auto& faces = rs.Faces();
    if (faces.empty()) {
        return nullptr;
    }

    float max_dist = rs.GetMaxDist();
    if (max_dist == 0) {
        return nullptr;
    }

    citygen::PolyBuilder builder;
    for (auto& f : faces) 
    {
        std::vector<sm::vec3> face_h;
        for (auto& p2 : f)
        {
            float dist = rs.QueryDist(p2);

            sm::vec3 p3(p2.x, dist * distance / max_dist, p2.y);
            p3.y += y;
            p3 = mat * p3;

            face_h.push_back(p3);
        }
        builder.AddFace(face_h);
    }

    auto polytope = builder.CreatePolytope();

    polytope->GetTopoPoly();
    polytope->BuildFromTopo();

    return polytope;
}

}