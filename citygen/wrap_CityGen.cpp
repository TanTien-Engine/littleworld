#include "wrap_CityGen.h"
#include "TensorField.h"
#include "Streets.h"
#include "Block.h"
#include "Parcels.h"
#include "modules/script/Proxy.h"
#include "modules/script/TransHelper.h"
#include "modules/graphics/Graphics.h"

#include <unirender/Texture.h>
#include <geoshape/Triangles.h>
#include <SM_Polyline.h>

#include <string>

namespace
{

void w_Streets_allocate()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
    auto tf = std::make_shared<citygen::TensorField>(*tex);

    std::vector<sm::vec2> border;

    auto poly = ves_toforeign(2);
    if (poly)
    {
        auto tris = ((tt::Proxy<gs::Triangles>*)poly)->obj;
        border = tris->GetBorder();
        border.push_back(border.front());
    }
    else
    {
        float min = 0.0f;
        float max = 1.0f;
        border = {
            { min, min },
            { max, min },
            { max, max },
            { min, max },
            { min, min },
        };
    }

    auto st = std::make_shared<citygen::Streets>(tf, border);

    auto proxy = (tt::Proxy<citygen::Streets>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<citygen::Streets>));
    proxy->obj = st;
}

int w_Streets_finalize(void* data)
{
    auto proxy = (tt::Proxy<citygen::Streets>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<citygen::Streets>);
}

void w_Streets_build_streamlines()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto num = (int)ves_tonumber(1);
    st->BuildStreamlines(num);
}

void w_Streets_build_topology()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    st->BuildTopology();
}

void return_points(const std::vector<std::vector<sm::vec2>>& points)
{
    ves_newlist(int(points.size()));
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

void w_Streets_get_major_paths()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto& major_paths = st->GetMajorPaths();

    std::vector<std::vector<sm::vec2>> points;
    for (auto& path : major_paths) {
        points.push_back(path->GetPoints());
    }
    
    ves_pop(1);

    return_points(points);
}

void w_Streets_get_minor_paths()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto& minor_paths = st->GetMinorPaths();

    std::vector<std::vector<sm::vec2>> points;
    for (auto& path : minor_paths) {
        points.push_back(path->GetPoints());
    }

    ves_pop(1);

    return_points(points);
}

void w_Streets_get_nodes()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto nodes = st->GetNodes();

    ves_pop(1);

    ves_newlist(int(nodes.size() * 2));

    for (int i = 0, n = (int)(nodes.size()); i < n; ++i) {
        for (int j = 0; j < 2; ++j) {
            ves_pushnumber(nodes[i].xy[j]);
            ves_seti(-2, i * 2 + j);
            ves_pop(1);
        }
    }
}

void w_Streets_get_polygons()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto polygons = st->GetPolygons();

    ves_pop(1);

    return_points(polygons);
}

void w_Streets_set_seed()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto seed = (float)ves_tonumber(1);
    st->SetSeed(seed);
}

void w_Block_allocate()
{
    auto tris = ((tt::Proxy<gs::Triangles>*)ves_toforeign(1))->obj;
    auto block = std::make_shared<citygen::Block>(tris->GetBorder());

    auto proxy = (tt::Proxy<citygen::Block>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<citygen::Block>));
    proxy->obj = block;
}

int w_Block_finalize(void* data)
{
    auto proxy = (tt::Proxy<citygen::Block>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<citygen::Block>);
}

void w_Block_offset()
{
    auto block = ((tt::Proxy<citygen::Block>*)ves_toforeign(0))->obj;
    auto dist = (float)ves_tonumber(1);

    auto borders = block->Offset(dist);

    ves_pop(2);

    if (borders.empty()) {
        ves_newlist(0);
        return;
    }

    ves_newlist(int(borders[0].size() * 2));

    for (int i = 0, n = (int)(borders[0].size()); i < n; ++i) {
        for (int j = 0; j < 2; ++j) {
            ves_pushnumber(borders[0][i].xy[j]);
            ves_seti(-2, i * 2 + j);
            ves_pop(1);
        }
    }
}

void w_Block_get_border()
{
    auto block = ((tt::Proxy<citygen::Block>*)ves_toforeign(0))->obj;
    auto borders = block->GetBorder();

    ves_pop(1);

    ves_newlist(int(borders[0].size() * 2));

    for (int i = 0, n = (int)(borders[0].size()); i < n; ++i) {
        for (int j = 0; j < 2; ++j) {
            ves_pushnumber(borders[0][i].xy[j]);
            ves_seti(-2, i * 2 + j);
            ves_pop(1);
        }
    }
}

void w_Parcels_allocate()
{
    auto tris = ((tt::Proxy<gs::Triangles>*)ves_toforeign(1))->obj;
    auto parcels = std::make_shared<citygen::Parcels>(tris->GetBorder());

    auto proxy = (tt::Proxy<citygen::Parcels>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<citygen::Parcels>));
    proxy->obj = parcels;
}

int w_Parcels_finalize(void* data)
{
    auto proxy = (tt::Proxy<citygen::Parcels>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<citygen::Parcels>);
}

void w_Parcels_build()
{
    auto parcels = ((tt::Proxy<citygen::Parcels>*)ves_toforeign(0))->obj;
    auto max_len = (float)ves_tonumber(1);
    parcels->Build(max_len);
}

void w_Parcels_offset()
{
    auto parcels = ((tt::Proxy<citygen::Parcels>*)ves_toforeign(0))->obj;
    auto dist = (float)ves_tonumber(1);
    parcels->Offset(dist);
}

void w_Parcels_get_polygons()
{
    auto parcels = ((tt::Proxy<citygen::Parcels>*)ves_toforeign(0))->obj;
    auto polygons = parcels->GetPolygons();

    ves_pop(1);

    return_points(polygons);
}

void w_Parcels_set_seed()
{
    auto parcels = ((tt::Proxy<citygen::Parcels>*)ves_toforeign(0))->obj;
    auto seed = (float)ves_tonumber(1);
    parcels->SetSeed(seed);
}

void w_GeometryTools_polyline_expand()
{
    auto polyline = tt::list_to_vec2_array(1);
    float offset = (float)ves_tonumber(2);

    bool is_closed = polyline.size() > 1 && polyline.front() == polyline.back();
    auto polylines = sm::polyline_expand(polyline, offset, is_closed);

    ves_pop(3);

    return_points(polylines);
}

}

namespace citygen
{

VesselForeignMethodFn CityGenBindMethod(const char* signature)
{
    if (strcmp(signature, "Streets.build_streamlines(_)") == 0) return w_Streets_build_streamlines;
    if (strcmp(signature, "Streets.build_topology()") == 0) return w_Streets_build_topology;
    if (strcmp(signature, "Streets.get_major_paths()") == 0) return w_Streets_get_major_paths;
    if (strcmp(signature, "Streets.get_minor_paths()") == 0) return w_Streets_get_minor_paths;
    if (strcmp(signature, "Streets.get_nodes()") == 0) return w_Streets_get_nodes;
    if (strcmp(signature, "Streets.get_polygons()") == 0) return w_Streets_get_polygons;
    if (strcmp(signature, "Streets.set_seed(_)") == 0) return w_Streets_set_seed;

    if (strcmp(signature, "Block.offset(_)") == 0) return w_Block_offset;
    if (strcmp(signature, "Block.get_border()") == 0) return w_Block_get_border;

    if (strcmp(signature, "Parcels.build(_)") == 0) return w_Parcels_build;
    if (strcmp(signature, "Parcels.offset(_)") == 0) return w_Parcels_offset;
    if (strcmp(signature, "Parcels.get_polygons()") == 0) return w_Parcels_get_polygons;
    if (strcmp(signature, "Parcels.set_seed(_)") == 0) return w_Parcels_set_seed;

    if (strcmp(signature, "static GeometryTools.polyline_expand(_,_)") == 0) return w_GeometryTools_polyline_expand;

	return nullptr;
}

void CityGenBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "Streets") == 0)
    {
        methods->allocate = w_Streets_allocate;
        methods->finalize = w_Streets_finalize;
        return;
    }

    if (strcmp(class_name, "Block") == 0)
    {
        methods->allocate = w_Block_allocate;
        methods->finalize = w_Block_finalize;
        return;
    }

    if (strcmp(class_name, "Parcels") == 0)
    {
        methods->allocate = w_Parcels_allocate;
        methods->finalize = w_Parcels_finalize;
        return;
    }
}

}