#include "wrap_ArchGen.h"
#include "MeshBuilder.h"
#include "modules/render/Render.h"
#include "modules/script/TransHelper.h"

#include <polymesh3/Polytope.h>
#include <unirender/VertexArray.h>

#include <string.h>

namespace
{

void w_TextureMapping_allocate()
{
    auto uv_map = std::make_shared<pm3::TextureMapping>();

	uv_map->sys.x_axis = tt::map_to_vec3(1);
	uv_map->sys.y_axis = tt::map_to_vec3(2);

	uv_map->scale  = tt::map_to_vec2(3);
	uv_map->offset = tt::map_to_vec2(4); 

    auto proxy = (tt::Proxy<pm3::TextureMapping>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::TextureMapping>));
    proxy->obj = uv_map;
}

int w_TextureMapping_finalize(void* data)
{
    auto proxy = (tt::Proxy<pm3::TextureMapping>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<pm3::TextureMapping>);
}

void w_TextureMapping_clone()
{
	auto src = ((tt::Proxy<pm3::TextureMapping>*)ves_toforeign(0))->obj;
	auto dst = std::make_shared<pm3::TextureMapping>(*src);

	ves_pop(ves_argnum());
	auto proxy = (tt::Proxy<pm3::TextureMapping>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<pm3::TextureMapping>));
	proxy->obj = dst;
}

void w_GeometryTools_build_mesh()
{
	std::vector<std::shared_ptr<pm3::Polytope>> polys;
	tt::list_to_foreigns(1, polys);

	std::shared_ptr<pm3::TextureMapping> uv_map = nullptr;
	if (ves_toforeign(2)) {
		uv_map = ((tt::Proxy<pm3::TextureMapping>*)(ves_toforeign(2)))->obj;
	}

	auto dev = tt::Render::Instance()->Device();
	auto va = archgen::MeshBuilder::Gen(*dev, polys, uv_map);
	if (va) {
		ves_pop(ves_argnum());

		ves_pushnil();
		ves_import_class("render", "VertexArray");
		auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::VertexArray>));
		proxy->obj = va;
		ves_pop(1);
	} else {
		ves_set_nil(0);
	}
}

void w_GeometryTools_build_mesh_from_file()
{
	auto filepath = ves_tostring(1);

	auto dev = tt::Render::Instance()->Device();
	auto va = archgen::MeshBuilder::Gen(*dev, filepath);
	if (va) {
		ves_pop(ves_argnum());

		ves_pushnil();
		ves_import_class("render", "VertexArray");
		auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::VertexArray>));
		proxy->obj = va;
		ves_pop(1);
	} else {
		ves_set_nil(0);
	}
}

}

namespace archgen
{

VesselForeignMethodFn ArchGenBindMethod(const char* signature)
{
	if (strcmp(signature, "TextureMapping.clone()") == 0) return w_TextureMapping_clone;

	if (strcmp(signature, "static GeometryTools.build_mesh(_,_)") == 0) return w_GeometryTools_build_mesh;
	if (strcmp(signature, "static GeometryTools.build_mesh(_)") == 0) return w_GeometryTools_build_mesh_from_file;

	return nullptr;
}

void ArchGenBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
	if (strcmp(class_name, "TextureMapping") == 0)
	{
		methods->allocate = w_TextureMapping_allocate;
		methods->finalize = w_TextureMapping_finalize;
		return;
	}
}

}