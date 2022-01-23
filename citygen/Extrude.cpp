#include "Extrude.h"
#include "StraightSkeleton.h"

#include <geoshape/Polygon2D.h>
#include <polymesh3/Polytope.h>
#include <SM_Calc.h>

namespace
{

class PolyBuilder
{
public:
	PolyBuilder() {}

    void AddFace(const std::vector<sm::vec2>& border, const std::vector<std::vector<sm::vec2>>& holes)
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

    void AddFace(const std::vector<sm::vec3>& border)
    {
        if (border.empty()) {
            return;
        }

        auto face = std::make_shared<pm3::Polytope::Face>();

        face->border = AddPoints(border);

        m_faces.push_back(face);
    }

    std::shared_ptr<pm3::Polytope> CreatePolytope() const
    {
        return std::make_shared<pm3::Polytope>(m_verts, m_faces);
    }

    auto& GetVertices() const { return m_verts; }

private:
    std::vector<size_t> AddPoints(const std::vector<sm::vec2>& points)
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

    std::vector<size_t> AddPoints(const std::vector<sm::vec3>& points)
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

private:
    std::vector<pm3::Polytope::PointPtr> m_verts;
    std::vector<pm3::Polytope::FacePtr>  m_faces;

	std::map<sm::vec2, size_t> m_pos2idx;

}; // PolyBuilder

std::map<sm::vec2, float> CalcPosHeight(const citygen::StraightSkeleton& ss)
{
    auto path_tree = ss.GetPathTree();
    auto path_root = path_tree->GetRoot();
    assert(path_root);

	std::map<sm::vec2, float> ret;

	std::queue<std::shared_ptr<citygen::StraightSkeleton::Node>> buf;
	buf.push(path_root->nodes[0]);
	buf.push(path_root->nodes[1]);

	std::set<std::shared_ptr<citygen::StraightSkeleton::Node>> old_set;
	old_set.insert(path_root->nodes[0]);
	old_set.insert(path_root->nodes[1]);

	while (!buf.empty())
	{
		auto n = buf.front(); buf.pop();

		// fixme
		float dist = n->dist;
		if (n == path_root->nodes[0] || n == path_root->nodes[1]) {
			dist = std::max(path_root->nodes[0]->dist, path_root->nodes[1]->dist);
		}
		ret.insert({ n->pos, dist });

		for (auto& p : n->paths) 
		{
			auto o = n == p->nodes[0] ? p->nodes[1] : p->nodes[0];
			if (old_set.find(o) == old_set.end()) {
				buf.push(o);
				old_set.insert(o);
			}
		}
	}

	return ret;
}

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

    StraightSkeleton ss(polygon->GetVertices());
    auto faces = ss.Faces();

    if (faces.empty()) {
        return Face(polygon, distance);
    }

    auto pos2dist = CalcPosHeight(ss);

    for (auto& f : faces) 
    {
        std::vector<sm::vec3> face_h;
        for (auto& p : f)
        {
            float dist = 0.0f;

            auto itr = pos2dist.find(p);
            if (itr == pos2dist.end()) 
            {
                float nearest = FLT_MAX;
                float nearest_dist = 0;
                for (auto itr : pos2dist) 
                {
                    float d = sm::dis_pos_to_pos(p, itr.first);
                    if (d < nearest) {
                        nearest = d;
                        nearest_dist = itr.second;
                    }
                }
                dist = nearest_dist;
            } 
            else 
            {
                dist = itr->second;
            }

            face_h.push_back({ p.x, dist * 0.5f, p.y });
        }
        builder.AddFace(face_h);
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