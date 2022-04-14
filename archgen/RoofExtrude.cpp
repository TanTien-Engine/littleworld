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

        std::reverse(face_h.begin(), face_h.end());

        builder.AddFace(face_h);
    }

    auto polytope = builder.CreatePolytope();

    polytope->GetTopoPoly();
    polytope->BuildFromTopo();

    return polytope;
}

std::shared_ptr<pm3::Polytope>
RoofExtrude::Pyramid(const std::shared_ptr<pm3::Polytope>& poly, float distance)
{
    if (poly->Faces().size() != 1) {
        return nullptr;
    }

    auto face = poly->Faces().front();

    sm::vec3 normal;
    if (!poly->CalcFaceNormal(*face, normal)) {
        return nullptr;
    }

    sm::vec3 center;

    if (face->border.size() < 3) {
        return nullptr;
    }

    auto& pts = poly->Points();
    for (auto idx : face->border) {
        center += pts[idx]->pos;
    }
    center /= face->border.size();

    center += normal * distance;

    citygen::PolyBuilder builder;
    for (int i = 0, n = face->border.size(); i < n; ++i)
    {
        std::vector<sm::vec3> face_h;
        face_h.push_back(pts[face->border[i]]->pos);
        face_h.push_back(pts[face->border[(i + 1) % n]]->pos);
        face_h.push_back(center);

        builder.AddFace(face_h);
    }

    auto polytope = builder.CreatePolytope();

    polytope->GetTopoPoly();
    polytope->BuildFromTopo();

    return polytope;
}

std::shared_ptr<pm3::Polytope>
RoofExtrude::Shed(const std::shared_ptr<pm3::Polytope>& poly, float distance, int index)
{
    if (poly->Faces().size() != 1) {
        return nullptr;
    }

    auto face = poly->Faces().front();

    sm::vec3 normal;
    if (!poly->CalcFaceNormal(*face, normal)) {
        return nullptr;
    }

    if (face->border.size() < 3) {
        return nullptr;
    }

    std::vector<sm::vec3> border;
    border.reserve(face->border.size());

    auto& pts = poly->Points();
    for (auto idx : face->border) {
        border.push_back(pts[idx]->pos);
    }

    auto& p0 = border[index % border.size()];
    auto& p1 = border[(index + 1) % border.size()];

    sm::vec3 y_dir = normal;
    sm::vec3 x_dir = (p1 - p0).Normalized();
    sm::vec3 z_dir = x_dir.Cross(y_dir);

    sm::mat4 mt(sm::mat3(x_dir, y_dir, z_dir));
    mt.Translate(p0.x, p0.y, p0.z);

    std::vector<sm::vec3> rot_border;
    auto inv_mt = mt.Inverted();
    rot_border.reserve(border.size());
    for (auto& p : border) {
        rot_border.push_back(inv_mt * p);
    }

    float z_min = FLT_MAX, z_max = -FLT_MAX;
    for (auto& p : rot_border) 
    {
        if (p.z < z_min) {
            z_min = p.z;
        }
        if (p.z > z_max) {
            z_max = p.z;
        }
    }

    std::vector<sm::vec3> roof_border;
    roof_border.reserve(border.size());
    for (int i = 0, n = border.size(); i < n; ++i) 
    {
        const float d = (rot_border[i].z - z_min) / (z_max - z_min);
        const auto p = border[i] + normal * distance * d;
        roof_border.push_back(p);
    }

    citygen::PolyBuilder builder;

    for (int i = 0, n = face->border.size(); i < n; ++i)
    {
        auto& b0 = border[i];
        auto& b1 = border[(i + 1) % n];
        auto& t0 = roof_border[i];
        auto& t1 = roof_border[(i + 1) % n];

        std::vector<sm::vec3> face_h;
        face_h.push_back(b0);
        face_h.push_back(b1);
        if (t1 != b1) {
            face_h.push_back(t1);
        }
        if (t0 != b0) {
            face_h.push_back(t0);
        }

        if (face_h.size() >= 3) {
            builder.AddFace(face_h);
        }
    }

    builder.AddFace(roof_border);

    auto polytope = builder.CreatePolytope();

    polytope->GetTopoPoly();
    polytope->BuildFromTopo();

    return polytope;
}

std::shared_ptr<pm3::Polytope>
RoofExtrude::Gable(const std::shared_ptr<pm3::Polytope>& poly, float distance)
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

    std::map<sm::vec2, sm::vec2> ext_ridge;
    for (auto& f : faces)
    {
        if (f.size() == 3) 
        {
            if (std::find(border.begin(), border.end(), f[0]) == border.end()) {
                ext_ridge.insert({ f[0], (f[1] + f[2]) * 0.5f });
            } else if (std::find(border.begin(), border.end(), f[1]) == border.end()) {
                ext_ridge.insert({ f[1], (f[0] + f[2]) * 0.5f });
            } else if (std::find(border.begin(), border.end(), f[2]) == border.end()) {
                ext_ridge.insert({ f[2], (f[0] + f[1]) * 0.5f });
            }
        }
    }

    citygen::PolyBuilder builder;
    for (auto& f : faces) 
    {
        std::vector<sm::vec3> face_h;
        for (auto& p2 : f)
        {
            float dist = rs.QueryDist(p2);

            auto _p2 = p2;
            auto itr = ext_ridge.find(p2);
            if (itr != ext_ridge.end()) {
                _p2 = itr->second;
            }

            sm::vec3 p3(_p2.x, dist * distance / max_dist, _p2.y);
            p3.y += y;
            p3 = mat * p3;

            face_h.push_back(p3);
        }

        std::reverse(face_h.begin(), face_h.end());

        builder.AddFace(face_h);
    }

    auto polytope = builder.CreatePolytope();

    polytope->GetTopoPoly();
    polytope->BuildFromTopo();

    return polytope;
}

}