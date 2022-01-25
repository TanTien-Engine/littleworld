#include "wrap_CityGen.h"
#include "TensorField.h"
#include "Streets.h"
#include "ParcelsOBB.h"
#include "ParcelsSS.h"
#include "Reshape.h"
#include "Extrude.h"
#include "Math.h"
#include "modules/script/Proxy.h"
#include "modules/script/TransHelper.h"
#include "modules/graphics/Graphics.h"

#include <unirender/Texture.h>
#include <geoshape/Polygon2D.h>
#include <geoshape/Polyline2D.h>
#include <polymesh3/Polytope.h>
#include <SM_Polyline.h>
#include <SM_Test.h>
#include <SM_DouglasPeucker.h>

#include <string>

namespace
{

void return_multi_polylines(const std::vector<std::vector<sm::vec2>>& polylines)
{
    ves_pop(ves_argnum());

    const int num = (int)(polylines.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        auto poly = std::make_shared<gs::Polyline2D>();
        poly->SetVertices(polylines[i]);

        ves_pushnil();
        ves_import_class("geometry", "Polyline");
        auto proxy = (tt::Proxy<gs::Polyline2D>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<gs::Polyline2D>));
        proxy->obj = poly;
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void return_polygon(const std::vector<std::vector<sm::vec2>>& polygon)
{
    if (polygon.empty()) 
    {
        ves_set_nil(0);
    }
    else
    {
        auto poly = std::make_shared<gs::Polygon2D>();
        poly->SetVertices(polygon[0]);
        for (int i = 1, n = polygon.size(); i < n; ++i) {
            poly->AddHole(polygon[i]);
        }

        ves_pop(ves_argnum());

        ves_pushnil();
        ves_import_class("geometry", "Polygon");
        auto proxy = (tt::Proxy<gs::Polygon2D>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<gs::Polygon2D>));
        proxy->obj = poly;
        ves_pop(1);
    }
}

void return_multi_polygons(const std::vector<std::vector<sm::vec2>>& polygons)
{
    ves_pop(ves_argnum());

    const int num = (int)(polygons.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        auto poly = std::make_shared<gs::Polygon2D>();
        poly->SetVertices(polygons[i]);

        ves_pushnil();
        ves_import_class("geometry", "Polygon");
        auto proxy = (tt::Proxy<gs::Polygon2D>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<gs::Polygon2D>));
        proxy->obj = poly;
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

void w_Streets_allocate()
{
    auto tex = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
    auto tf = std::make_shared<citygen::TensorField>(*tex);

    std::vector<sm::vec2> border;

    auto poly = ves_toforeign(2);
    if (poly)
    {
        auto poly2d = ((tt::Proxy<gs::Polygon2D>*)poly)->obj;
        border = poly2d->GetVertices();
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
    auto random = ves_toboolean(2);
    st->BuildStreamlines(num, random);
}

void w_Streets_build_topology()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    st->BuildTopology();
}

void w_Streets_get_major_paths()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto& major_paths = st->GetMajorPaths();

    std::vector<std::vector<sm::vec2>> polylines;
    for (auto& path : major_paths) {
        auto fixed = citygen::Math::RemoveDuplicatedPos(path->GetGraphPoints());
        polylines.push_back(fixed);
    }
    return_multi_polylines(polylines);
}

void w_Streets_get_minor_paths()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto& minor_paths = st->GetMinorPaths();

    std::vector<std::vector<sm::vec2>> polylines;
    for (auto& path : minor_paths) {
        auto fixed = citygen::Math::RemoveDuplicatedPos(path->GetGraphPoints());
        polylines.push_back(fixed);
    }
    return_multi_polylines(polylines);
}

void w_Streets_get_nodes()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto nodes = st->GetNodes();
    tt::return_list(nodes);
}

void w_Streets_get_polygons()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto polygons = st->GetPolygons();
    return_multi_polygons(polygons);
}

void w_Streets_set_seed()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto seed = (float)ves_tonumber(1);
    st->SetSeed(seed);
}

void w_Streets_set_nodes()
{
    auto st = ((tt::Proxy<citygen::Streets>*)ves_toforeign(0))->obj;
    auto pts = tt::list_to_vec2_array(1);
    bool dirty = st->TranslateNodes(pts);
    ves_set_boolean(0, dirty);
}

void w_ParcelsOBB_allocate()
{
    auto poly = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    auto parcels = std::make_shared<citygen::ParcelsOBB>(poly->GetVertices());

    auto proxy = (tt::Proxy<citygen::ParcelsOBB>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<citygen::ParcelsOBB>));
    proxy->obj = parcels;
}

int w_ParcelsOBB_finalize(void* data)
{
    auto proxy = (tt::Proxy<citygen::ParcelsOBB>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<citygen::ParcelsOBB>);
}

void w_ParcelsOBB_build()
{
    auto parcels = ((tt::Proxy<citygen::ParcelsOBB>*)ves_toforeign(0))->obj;
    auto max_len = (float)ves_tonumber(1);
    parcels->Build(max_len);
}

void w_ParcelsOBB_get_polygons()
{
    auto parcels = ((tt::Proxy<citygen::ParcelsOBB>*)ves_toforeign(0))->obj;
    auto polygons = parcels->GetPolygons();
    return_multi_polygons(polygons);
}

void w_ParcelsOBB_set_seed()
{
    auto parcels = ((tt::Proxy<citygen::ParcelsOBB>*)ves_toforeign(0))->obj;
    auto seed = (float)ves_tonumber(1);
    parcels->SetSeed(seed);
}

void w_ParcelsSS_allocate()
{
    auto poly = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    auto parcels = std::make_shared<citygen::ParcelsSS>(poly->GetVertices());

    auto proxy = (tt::Proxy<citygen::ParcelsSS>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<citygen::ParcelsSS>));
    proxy->obj = parcels;
}

int w_ParcelsSS_finalize(void* data)
{
    auto proxy = (tt::Proxy<citygen::ParcelsSS>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<citygen::ParcelsSS>);
}

void w_ParcelsSS_build()
{
    auto parcels = ((tt::Proxy<citygen::ParcelsSS>*)ves_toforeign(0))->obj;
    auto max_len = (float)ves_tonumber(1);
    parcels->Build(max_len);
}

void w_ParcelsSS_get_polygons()
{
    auto parcels = ((tt::Proxy<citygen::ParcelsSS>*)ves_toforeign(0))->obj;
    auto polygons = parcels->GetPolygons();
    return_multi_polygons(polygons);
}

void w_ParcelsSS_set_seed()
{
    auto parcels = ((tt::Proxy<citygen::ParcelsSS>*)ves_toforeign(0))->obj;
    auto seed = (float)ves_tonumber(1);
    parcels->SetSeed(seed);
}

void w_GeometryTools_polyline_offset()
{
    auto polygon = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    float distance = (float)ves_tonumber(2);
    bool is_closed = ves_toboolean(3);

    auto polylines = sm::polyline_offset(polygon->GetVertices(), distance, is_closed);
    return_polygon(polylines);
}

void w_GeometryTools_polyline_expand()
{
    auto polyline = ((tt::Proxy<gs::Polyline2D>*)ves_toforeign(1))->obj;
    float offset = (float)ves_tonumber(2);

    auto& vertices = polyline->GetVertices();
    bool is_closed = vertices.size() > 1 && vertices.front() == vertices.back();
    auto polylines = sm::polyline_expand(vertices, offset, is_closed);

    // fixme: build brush bug, simplify first
    for (auto& poly : polylines) {
        std::vector<sm::vec2> pts;
        sm::douglas_peucker(poly, 0.0001f, pts);
        poly = pts;
    }

    return_polygon(polylines);
}

void w_GeometryTools_shape_l()
{
    auto polygon = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    float front_width = (float)ves_tonumber(2);
    float left_width = (float)ves_tonumber(3);
    bool remainder = ves_toboolean(4);

    citygen::Reshape rs(polygon);
    return_polygon(rs.ShapeL(front_width, left_width, remainder));
}

void w_GeometryTools_shape_u()
{
    auto polygon = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    float front_width = (float)ves_tonumber(2);
    float left_width = (float)ves_tonumber(3);
    float right_width = (float)ves_tonumber(4);
    bool remainder = ves_toboolean(5);

    citygen::Reshape rs(polygon);
    return_polygon(rs.ShapeU(front_width, left_width, right_width, remainder));
}

void w_GeometryTools_shape_o()
{
    auto polygon = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    float front_width = (float)ves_tonumber(2);
    float back_width = (float)ves_tonumber(3);
    float left_width = (float)ves_tonumber(4);
    float right_width = (float)ves_tonumber(5);
    bool remainder = ves_toboolean(6);

    citygen::Reshape rs(polygon);
    return_polygon(rs.ShapeO(front_width, back_width, left_width, right_width, remainder));
}

void w_GeometryTools_is_counterclockwise()
{
    auto polygon = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    ves_set_boolean(0, !sm::is_polygon_clockwise(polygon->GetVertices()));
}

void w_GeometryTools_polygon_extrude_face()
{
    auto polygon = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    float distance = (float)ves_tonumber(2);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("geometry", "Polytope");
    auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
    proxy->obj = citygen::Extrude::Face(polygon, distance);
    ves_pop(1);
}

void w_GeometryTools_polygon_extrude_skeleton()
{
    auto polygon = ((tt::Proxy<gs::Polygon2D>*)ves_toforeign(1))->obj;
    float distance = (float)ves_tonumber(2);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("geometry", "Polytope");
    auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
    proxy->obj = citygen::Extrude::Skeleton(polygon, distance);
    ves_pop(1);
}

}

namespace citygen
{

VesselForeignMethodFn CityGenBindMethod(const char* signature)
{
    if (strcmp(signature, "Streets.build_streamlines(_,_)") == 0) return w_Streets_build_streamlines;
    if (strcmp(signature, "Streets.build_topology()") == 0) return w_Streets_build_topology;
    if (strcmp(signature, "Streets.get_major_paths()") == 0) return w_Streets_get_major_paths;
    if (strcmp(signature, "Streets.get_minor_paths()") == 0) return w_Streets_get_minor_paths;
    if (strcmp(signature, "Streets.get_nodes()") == 0) return w_Streets_get_nodes;
    if (strcmp(signature, "Streets.get_polygons()") == 0) return w_Streets_get_polygons;
    if (strcmp(signature, "Streets.set_seed(_)") == 0) return w_Streets_set_seed;
    if (strcmp(signature, "Streets.set_nodes(_)") == 0) return w_Streets_set_nodes;

    if (strcmp(signature, "ParcelsOBB.build(_)") == 0) return w_ParcelsOBB_build;
    if (strcmp(signature, "ParcelsOBB.get_polygons()") == 0) return w_ParcelsOBB_get_polygons;
    if (strcmp(signature, "ParcelsOBB.set_seed(_)") == 0) return w_ParcelsOBB_set_seed;

    if (strcmp(signature, "ParcelsSS.build(_)") == 0) return w_ParcelsSS_build;
    if (strcmp(signature, "ParcelsSS.get_polygons()") == 0) return w_ParcelsSS_get_polygons;
    if (strcmp(signature, "ParcelsSS.set_seed(_)") == 0) return w_ParcelsSS_set_seed;

    if (strcmp(signature, "static GeometryTools.polyline_offset(_,_,_)") == 0) return w_GeometryTools_polyline_offset;
    if (strcmp(signature, "static GeometryTools.polyline_expand(_,_)") == 0) return w_GeometryTools_polyline_expand;
    if (strcmp(signature, "static GeometryTools.shape_l(_,_,_,_)") == 0) return w_GeometryTools_shape_l;
    if (strcmp(signature, "static GeometryTools.shape_u(_,_,_,_,_)") == 0) return w_GeometryTools_shape_u;
    if (strcmp(signature, "static GeometryTools.shape_o(_,_,_,_,_,_)") == 0) return w_GeometryTools_shape_o;
    if (strcmp(signature, "static GeometryTools.is_counterclockwise(_)") == 0) return w_GeometryTools_is_counterclockwise;
    if (strcmp(signature, "static GeometryTools.polygon_extrude_face(_,_)") == 0) return w_GeometryTools_polygon_extrude_face;
    if (strcmp(signature, "static GeometryTools.polygon_extrude_skeleton(_,_)") == 0) return w_GeometryTools_polygon_extrude_skeleton;

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

    if (strcmp(class_name, "ParcelsOBB") == 0)
    {
        methods->allocate = w_ParcelsOBB_allocate;
        methods->finalize = w_ParcelsOBB_finalize;
        return;
    }

    if (strcmp(class_name, "ParcelsSS") == 0)
    {
        methods->allocate = w_ParcelsSS_allocate;
        methods->finalize = w_ParcelsSS_finalize;
        return;
    }
}

}