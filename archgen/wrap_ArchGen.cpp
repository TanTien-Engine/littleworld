#include "wrap_ArchGen.h"
#include "MeshBuilder.h"
#include "RoofEditor.h"
#include "ScopeTools.h"
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

	uv_map->uw_factor = ves_tonumber(5);

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

void w_MeshBuilder_build_mesh_from_poly()
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

	std::shared_ptr<pm3::TextureMapping> uv_map = nullptr;
	if (ves_toforeign(2)) {
		uv_map = ((tt::Proxy<pm3::TextureMapping>*)(ves_toforeign(2)))->obj;
	}

	auto dev = tt::Render::Instance()->Device();
	sm::cube aabb;
	auto va = archgen::MeshBuilder::Gen(*dev, filepath, uv_map, aabb);
	if (va) {
		return_mesh(va, aabb);
	} else {
		ves_set_nil(0);
	}
}

void w_ScopeTools_face_mapping()
{
	auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;
	auto face = ((tt::Proxy<pm3::Polytope::Face>*)ves_toforeign(2))->obj;

	sm::vec3 o, x, y;
	if (!archgen::ScopeTools::CalcFaceMapping(*poly, *face, o, x, y)) {
		ves_set_nil(0);
		return;
	}

	tt::return_list(std::vector<float>{ o.x, o.y, o.z, x.x, x.y, x.z, y.x, y.y, y.z });
}

void w_ScopeTools_calc_insert_mat()
{
	sm::cube* aabb = (sm::cube*)ves_toforeign(1);
	sm::mat4* scope = (sm::mat4*)ves_toforeign(2);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("maths", "Matrix44");
	sm::mat4* mat = (sm::mat4*)ves_set_newforeign(0, 1, sizeof(sm::mat4));
	*mat = archgen::ScopeTools::CalcInsertMat(*aabb, *scope);
	ves_pop(1);
}

void w_ScopeTools_get_scope_size()
{
	sm::mat4* scope = (sm::mat4*)ves_toforeign(1);

	sm::vec3 s, r, t;
	scope->Decompose(t, r, s);

	tt::return_list(std::vector<float>{ s.x, s.y, s.z });
}

void w_ScopeTools_comp_edges()
{
	auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;

	auto mats = archgen::ScopeTools::CalcEdgesMapping(*poly);
	
	ves_pop(ves_argnum());

	const int num = (int)(mats.size());
	ves_newlist(num);
	for (int i = 0; i < num; ++i)
	{
		ves_pushnil();
		ves_import_class("maths", "Matrix44");
		sm::mat4* mat = (sm::mat4*)ves_set_newforeign(1, 2, sizeof(sm::mat4));
		*mat = mats[i];
		ves_pop(1);
		ves_seti(-2, i);
		ves_pop(1);
	}
}

void w_ScopeTools_comp_face_edges()
{
	auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;

	auto mats = archgen::ScopeTools::CalcFaceEdgesMapping(*poly);
	
	ves_pop(ves_argnum());

	const int num = (int)(mats.size());
	ves_newlist(num);
	for (int i = 0; i < num; ++i)
	{
		ves_pushnil();
		ves_import_class("maths", "Matrix44");
		sm::mat4* mat = (sm::mat4*)ves_set_newforeign(1, 2, sizeof(sm::mat4));
		*mat = mats[i];
		ves_pop(1);
		ves_seti(-2, i);
		ves_pop(1);
	}
}

void w_ScopeTools_comp_roof_edges()
{
	auto roof = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;
	auto base = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(2))->obj;

	// eave, hip, valley, ridge
	const int groups_num = 4;
	std::vector<sm::mat4> mats[groups_num];
	archgen::ScopeTools::CalcRoofEdgesMapping(*roof, *base, mats[0], mats[1], mats[2], mats[3]);

	ves_pop(ves_argnum());

	ves_newlist(groups_num);
	for (int i = 0; i < groups_num; ++i)
	{
		const int num = mats[i].size();
		ves_newlist(num);

		for (int j = 0; j < num; ++j)
		{
			ves_pushnil();
			ves_import_class("maths", "Matrix44");
			sm::mat4* mat = (sm::mat4*)ves_set_newforeign(2, 3, sizeof(sm::mat4));
			*mat = mats[i][j];
			ves_pop(1);
			ves_seti(-2, j);
			ves_pop(1);
		}

		ves_seti(-2, i);
		ves_pop(1);
	}
}

void w_RoofEditor_hip()
{
	auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;
	float dist = ves_tonumber(2);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("geometry", "Polytope");
	auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
	proxy->obj = archgen::RoofEditor::Hip(poly, dist);
	ves_pop(1);
}

void w_RoofEditor_pyramid()
{
	auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;
	float dist = ves_tonumber(2);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("geometry", "Polytope");
	auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
	proxy->obj = archgen::RoofEditor::Pyramid(poly, dist);
	ves_pop(1);
}

void w_RoofEditor_shed()
{
	auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;
	float dist = ves_tonumber(2);
	int index = (int)ves_tonumber(3);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("geometry", "Polytope");
	auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
	proxy->obj = archgen::RoofEditor::Shed(poly, dist, index);
	ves_pop(1);
}

void w_RoofEditor_gable()
{
	auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;
	float dist = ves_tonumber(2);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("geometry", "Polytope");
	auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
	proxy->obj = archgen::RoofEditor::Gable(poly, dist);
	ves_pop(1);
}

void w_RoofEditor_offset()
{
	auto poly = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(1))->obj;
	auto base = ((tt::Proxy<pm3::Polytope>*)ves_toforeign(2))->obj;
	float dist = ves_tonumber(3);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("geometry", "Polytope");
	auto proxy = (tt::Proxy<pm3::Polytope>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<pm3::Polytope>));
	proxy->obj = archgen::RoofEditor::Offset(*poly, *base, dist);
	ves_pop(1);
}

}

namespace archgen
{

VesselForeignMethodFn ArchGenBindMethod(const char* signature)
{
	if (strcmp(signature, "TextureMapping.clone()") == 0) return w_TextureMapping_clone;

	if (strcmp(signature, "static MeshBuilder.build_mesh_from_poly(_,_)") == 0) return w_MeshBuilder_build_mesh_from_poly;
	if (strcmp(signature, "static MeshBuilder.build_mesh_from_file(_,_)") == 0) return w_MeshBuilder_build_mesh_from_file;

	if (strcmp(signature, "static ScopeTools.face_mapping(_,_)") == 0) return w_ScopeTools_face_mapping;
	if (strcmp(signature, "static ScopeTools.calc_insert_mat(_,_)") == 0) return w_ScopeTools_calc_insert_mat;
	if (strcmp(signature, "static ScopeTools.get_scope_size(_)") == 0) return w_ScopeTools_get_scope_size;
	if (strcmp(signature, "static ScopeTools.comp_edges(_)") == 0) return w_ScopeTools_comp_edges;
	if (strcmp(signature, "static ScopeTools.comp_face_edges(_)") == 0) return w_ScopeTools_comp_face_edges;
	if (strcmp(signature, "static ScopeTools.comp_roof_edges(_,_)") == 0) return w_ScopeTools_comp_roof_edges;

	if (strcmp(signature, "static RoofEditor.hip(_,_)") == 0) return w_RoofEditor_hip;
	if (strcmp(signature, "static RoofEditor.pyramid(_,_)") == 0) return w_RoofEditor_pyramid;
	if (strcmp(signature, "static RoofEditor.shed(_,_,_)") == 0) return w_RoofEditor_shed;
	if (strcmp(signature, "static RoofEditor.gable(_,_)") == 0) return w_RoofEditor_gable;
	if (strcmp(signature, "static RoofEditor.offset(_,_,_)") == 0) return w_RoofEditor_offset;

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