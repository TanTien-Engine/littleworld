#pragma once

#include <SM_Vector.h>
#include <polymesh3/Polytope.h>

#include <vector>
#include <memory>
#include <map>

namespace citygen
{

class PolyBuilder
{
public:
	PolyBuilder() {}

    void AddFace(const std::vector<sm::vec2>& border, const std::vector<std::vector<sm::vec2>>& holes);

    void AddFace(const std::vector<sm::vec3>& border);

    std::shared_ptr<pm3::Polytope> CreatePolytope() const;

    auto& GetVertices() const { return m_verts; }

private:
    std::vector<size_t> AddPoints(const std::vector<sm::vec2>& points);

    std::vector<size_t> AddPoints(const std::vector<sm::vec3>& points);

private:
    std::vector<pm3::Polytope::PointPtr> m_verts;
    std::vector<pm3::Polytope::FacePtr>  m_faces;

	std::map<sm::vec2, size_t> m_pos2idx;
    std::map<sm::vec3, size_t> m_pos3idx;

}; // PolyBuilder

}