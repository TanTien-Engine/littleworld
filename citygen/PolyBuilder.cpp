#include "PolyBuilder.h"

namespace citygen
{

void PolyBuilder::AddFace(const std::vector<sm::vec2>& border, const std::vector<std::vector<sm::vec2>>& holes)
{
    if (border.empty()) {
        return;
    }

    auto face = std::make_shared<pm3::Polytope::Face>();

    face->border = AddPoints(border);
    for (auto& hole : holes) {
        face->holes.push_back(AddPoints(hole));
    }

    m_faces.push_back(face);
}

void PolyBuilder::AddFace(const std::vector<sm::vec3>& border)
{
    if (border.empty()) {
        return;
    }

    auto face = std::make_shared<pm3::Polytope::Face>();

    face->border = AddPoints(border);

    m_faces.push_back(face);
}

std::shared_ptr<pm3::Polytope> PolyBuilder::CreatePolytope() const
{
    return std::make_shared<pm3::Polytope>(m_verts, m_faces);
}

std::vector<size_t> PolyBuilder::AddPoints(const std::vector<sm::vec2>& points)
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
            auto vert = std::make_shared<pm3::Polytope::Point>(sm::vec3(p.x, 0.0f, p.y));
            idx = m_verts.size();
            m_pos2idx.insert({ p, idx });
            m_verts.push_back(vert);
        }

        idxes.push_back(idx);
    }

    return idxes;
}

std::vector<size_t> PolyBuilder::AddPoints(const std::vector<sm::vec3>& points)
{
    std::vector<size_t> idxes;

    for (auto& p : points)
    {
        size_t idx = 0;

        auto p2 = sm::vec2(p.x, p.z);

        auto itr = m_pos2idx.find(p2);
        if (itr != m_pos2idx.end()) 
        {
            idx = itr->second;
        } 
        else 
        {
            auto vert = std::make_shared<pm3::Polytope::Point>(p);
            idx = m_verts.size();
            m_pos2idx.insert({ p2, idx });
            m_verts.push_back(vert);
        }

        idxes.push_back(idx);
    }

    return idxes;
}

}