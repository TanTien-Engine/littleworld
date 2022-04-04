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

void return_mesh(const std::shared_ptr<ur::VertexArray>& va, const sm::cube& aabb)
{
	ves_pop(ves_argnum());

	ves_newmap();
	{
		ves_pushnil();
		ves_import_class("render", "VertexArray");
		auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<ur::VertexArray>));
		proxy->obj = va;
		ves_pop(1);
		ves_setfield(-2, "va");
		ves_pop(1);
	}
	{
		ves_pushnil();
		ves_import_class("maths", "Cube");
		sm::cube* cube = (sm::cube*)ves_set_newforeign(1, 2, sizeof(sm::cube));
		*cube = aabb;
		ves_pop(1);
		ves_setfield(-2, "aabb");
		ves_pop(1);
	}
}

void w_MeshBuilder_build_mesh()
{
	std::vector<std::shared_ptr<pm3::Polytope>> polys;
	tt::list_to_foreigns(1, polys);

	std::shared_ptr<pm3::TextureMapping> uv_map = nullptr;
	if (ves_toforeign(2)) {
		uv_map = ((tt::Proxy<pm3::TextureMapping>*)(ves_toforeign(2)))->obj;
	}

	auto dev = tt::Render::Instance()->Device();
	sm::cube aabb;
	auto va = archgen::MeshBuilder::Gen(*dev, polys, uv_map, aabb);
	if (va) {
		return_mesh(va, aabb);
	} else {
		ves_set_nil(0);
	}
}

void w_MeshBuilder_build_mesh_from_file()
{
	auto filepath = ves_tostring(1);

	auto dev = tt::Render::Instance()->Device();
	sm::cube aabb;
	auto va = archgen::MeshBuilder::Gen(*dev, filepath, aabb);
	if (va) {
		return_mesh(va, aabb);
	} else {
		ves_set_nil(0);
	}
}

void w_ArchTools_calc_geo_mat()
{
	sm::cube* aabb = (sm::cube*)ves_toforeign(1);
	sm::mat4* scope = (sm::mat4*)ves_toforeign(2);

	auto c = aabb->Center();
	auto mat_t = sm::mat4::Translated(c.x, c.y, c.z);

	auto sz = aabb->Size();
	auto mat_s = sm::mat4::Scaled(1.0f / sz.x, 1.0f / sz.y, 1.0f / sz.z);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("maths", "Matrix44");
	sm::mat4* mat = (sm::mat4*)ves_set_newforeign(0, 1, sizeof(sm::mat4));
	*mat = *scope * mat_t * mat_s;
	ves_pop(1);
}

}

namespace archgen
{

VesselForeignMethodFn ArchGenBindMethod(const char* signature)
{
	if (strcmp(signature, "TextureMapping.clone()") == 0) return w_TextureMapping_clone;

	if (strcmp(signature, "static MeshBuilder.build_mesh(_,_)") == 0) return w_MeshBuilder_build_mesh;
	if (strcmp(signature, "static MeshBuilder.build_mesh(_)") == 0) return w_MeshBuilder_build_mesh_from_file;

	if (strcmp(signature, "static ArchTools.calc_geo_mat(_,_)") == 0) return w_ArchTools_calc_geo_mat;

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