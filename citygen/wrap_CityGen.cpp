#include "wrap_CityGen.h"
#include "modules/script/Proxy.h"
#include "TensorField.h"
#include "Network.h"

#include <unirender/Texture.h>

#include <string>

namespace
{

void w_Network_allocate()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
    auto tf = std::make_shared<citygen::TensorField>(*tex);
    auto nw = std::make_shared<citygen::Network>(tf);

    auto proxy = (tt::Proxy<citygen::Network>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<citygen::Network>));
    proxy->obj = nw;
}

int w_Network_finalize(void* data)
{
    auto proxy = (tt::Proxy<citygen::Network>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<citygen::Network>);
}

void w_Network_build_streamlines()
{
    auto nw = ((tt::Proxy<citygen::Network>*)ves_toforeign(0))->obj;
    auto num = (int)ves_tonumber(1);
    nw->BuildStreamlines(num);
}

void w_Network_build_topology()
{
    auto nw = ((tt::Proxy<citygen::Network>*)ves_toforeign(0))->obj;
    nw->BuildTopology();
}

void return_points(const std::vector<std::vector<sm::vec2>>& points)
{
    if (points.empty()) {
        ves_pushnil();
        return;
    }

    ves_newlist(points.size());
    for (int i_list = 0; i_list < points.size(); ++i_list)
    {
        ves_newlist(int(points[i_list].size() * 2));

        for (int i = 0, n = (int)(points[i_list].size()); i < n; ++i) {
            for (int j = 0; j < 2; ++j) {
                ves_pushnumber(points[i_list][i].xy[j]);
                ves_seti(-2, i * 2 + j);
                ves_pop(1);
            }
        }

        ves_seti(-2, i_list);
        ves_pop(1);
    }
}

void w_Network_get_major_paths()
{
    auto nw = ((tt::Proxy<citygen::Network>*)ves_toforeign(0))->obj;
    auto& major_paths = nw->GetMajorPaths();
    
    ves_pop(1);

    return_points(major_paths);
}

void w_Network_get_minor_paths()
{
    auto nw = ((tt::Proxy<citygen::Network>*)ves_toforeign(0))->obj;
    auto& minor_paths = nw->GetMinorPaths();

    ves_pop(1);

    return_points(minor_paths);
}

void w_Network_get_nodes()
{
    auto nw = ((tt::Proxy<citygen::Network>*)ves_toforeign(0))->obj;
    auto nodes = nw->GetNodes();

    ves_pop(1);

    if (nodes.empty()) {
        ves_pushnil();
        return;
    }

    ves_newlist(int(nodes.size() * 2));

    for (int i = 0, n = (int)(nodes.size()); i < n; ++i) {
        for (int j = 0; j < 2; ++j) {
            ves_pushnumber(nodes[i].xy[j]);
            ves_seti(-2, i * 2 + j);
            ves_pop(1);
        }
    }
}

void w_Network_get_polygons()
{
    auto nw = ((tt::Proxy<citygen::Network>*)ves_toforeign(0))->obj;
    auto polygons = nw->GetPolygons();

    ves_pop(1);

    return_points(polygons);
}

}

namespace citygen
{

VesselForeignMethodFn CityGenBindMethod(const char* signature)
{
    if (strcmp(signature, "Network.build_streamlines(_)") == 0) return w_Network_build_streamlines;
    if (strcmp(signature, "Network.build_topology()") == 0) return w_Network_build_topology;
    if (strcmp(signature, "Network.get_major_paths()") == 0) return w_Network_get_major_paths;
    if (strcmp(signature, "Network.get_minor_paths()") == 0) return w_Network_get_minor_paths;
    if (strcmp(signature, "Network.get_nodes()") == 0) return w_Network_get_nodes;
    if (strcmp(signature, "Network.get_polygons()") == 0) return w_Network_get_polygons;

	return nullptr;
}

void CityGenBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Network") == 0)
    {
        methods->allocate = w_Network_allocate;
        methods->finalize = w_Network_finalize;
        return;
    }
}

}