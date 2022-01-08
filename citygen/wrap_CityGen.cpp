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

void return_paths(const std::vector<std::shared_ptr<citygen::Network::Path>>& paths)
{
    if (paths.empty()) {
        ves_pushnil();
        return;
    }

    ves_newlist(paths.size());
    for (int i_path = 0; i_path < paths.size(); ++i_path)
    {
        ves_newlist(int(paths[i_path]->points.size() * 2));

        for (int i = 0, n = (int)(paths[i_path]->points.size()); i < n; ++i) {
            for (int j = 0; j < 2; ++j) {
                ves_pushnumber(paths[i_path]->points[i].xy[j]);
                ves_seti(-2, i * 2 + j);
                ves_pop(1);
            }
        }

        ves_seti(-2, i_path);
        ves_pop(1);
    }
}

void w_Network_get_major_paths()
{
    auto nw = ((tt::Proxy<citygen::Network>*)ves_toforeign(0))->obj;
    auto& major_paths = nw->GetMajorPaths();
    
    ves_pop(1);

    return_paths(major_paths);
}

void w_Network_get_minor_paths()
{
    auto nw = ((tt::Proxy<citygen::Network>*)ves_toforeign(0))->obj;
    auto& minor_paths = nw->GetMinorPaths();

    ves_pop(1);

    return_paths(minor_paths);
}

}

namespace citygen
{

VesselForeignMethodFn CityGenBindMethod(const char* signature)
{
    if (strcmp(signature, "Network.build_streamlines(_)") == 0) return w_Network_build_streamlines;
    if (strcmp(signature, "Network.get_major_paths()") == 0) return w_Network_get_major_paths;
    if (strcmp(signature, "Network.get_minor_paths()") == 0) return w_Network_get_minor_paths;

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