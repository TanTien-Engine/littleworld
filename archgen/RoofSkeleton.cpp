#include "RoofSkeleton.h"
#include "../citygen/StraightSkeleton.h"

#include <SM_Calc.h>

namespace archgen
{

RoofSkeleton::RoofSkeleton(const std::vector<sm::vec2>& polygon)
{
	m_ss = std::make_shared<citygen::StraightSkeleton>(polygon);

	m_faces = m_ss->Faces();

	CalcSkelDist();
}

float RoofSkeleton::QueryDist(const sm::vec2& p) const
{
    float dist = 0.0f;

    auto itr = m_pos2dist.find(p);
    if (itr == m_pos2dist.end()) 
    {
        float nearest = FLT_MAX;
        float nearest_dist = 0;
        for (auto itr : m_pos2dist) 
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

	return dist;
}

void RoofSkeleton::CalcSkelDist()
{
    auto path_tree = m_ss->GetPathTree();
    auto path_root = path_tree->GetRoot();
    assert(path_root);

	m_pos2dist.clear();
	m_max_dist = 0;

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
		m_pos2dist.insert({ n->pos, dist });

		if (dist > m_max_dist) {
			m_max_dist = dist;
		}

		for (auto& p : n->paths) 
		{
			auto o = n == p->nodes[0] ? p->nodes[1] : p->nodes[0];
			if (old_set.find(o) == old_set.end()) {
				buf.push(o);
				old_set.insert(o);
			}
		}
	}
}

}