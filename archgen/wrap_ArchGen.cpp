#include "wrap_ArchGen.h"
#include "MeshBuilder.h"
#include "modules/render/Render.h"
#include "modules/script/TransHelper.h"

#include <polymesh3/Polytope.h>
#include <unirender/VertexArray.h>
#include <SM_Rect.h>

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

	sm::vec3 normal;
	if (!poly->CalcFaceNormal(*face, normal)) {
		ves_set_nil(0);
		return;
	}

	auto rot = sm::mat4(sm::Quaternion::CreateFromVectors(normal, sm::vec3(0, 0, 1)));

	auto& pts = poly->Points();

	std::vector<sm::vec3> rot_face;
	rot_face.reserve(face->border.size());
	for (auto idx : face->border) {
		rot_face.push_back(rot * pts[idx]->pos);
	}

	float z = 0.0f;
	for (auto& p : rot_face) {
		z += p.z;
	}
	z /= rot_face.size();

	sm::rect r;
	r.MakeEmpty();
	for (auto& p : rot_face) {
		r.Combine(sm::vec2(p.x, p.y));
	}

	auto inv_rot = rot.Inverted();

	sm::vec3 o = inv_rot * sm::vec3(r.xmin, r.ymin, z);
	sm::vec3 dx = inv_rot * sm::vec3(r.xmax, r.ymin, z) - o;
	sm::vec3 dy = inv_rot * sm::vec3(r.xmin, r.ymax, z) - o;

	const sm::vec3 EXPECT_Y(0, 1, 0);
	if (fabs(EXPECT_Y.Dot(dx)) - fabs(EXPECT_Y.Dot(dy)) > SM_LARGE_EPSILON) {
		std::swap(dx, dy);
	}

	tt::return_list(std::vector<float>{ o.x, o.y, o.z, dx.x, dx.y, dx.z, dy.x, dy.y, dy.z });
}

void w_ScopeTools_calc_insert_mat()
{
	sm::cube* aabb = (sm::cube*)ves_toforeign(1);
	sm::mat4* scope = (sm::mat4*)ves_toforeign(2);

	auto size = aabb->Size();
	const float sx = 1.0f / size.x;
	const float sy = 1.0f / size.y;
	const float sz = 1.0f / size.z;

	auto mat_s = sm::mat4::Scaled(sx, sy, sz);

	auto c = aabb->Center();
	auto mat_t = sm::mat4::Translated(-c.x, -c.y, -c.z);

	auto mat_o = sm::mat4::Translated(0.5f, 0.5f, 0.0f);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("maths", "Matrix44");
	sm::mat4* mat = (sm::mat4*)ves_set_newforeign(0, 1, sizeof(sm::mat4));
	*mat = *scope * mat_o * mat_s * mat_t;
	ves_pop(1);
}

void w_ScopeTools_get_scope_size()
{
	sm::mat4* scope = (sm::mat4*)ves_toforeign(1);

	sm::vec3 s, r, t;
	scope->Decompose(t, r, s);

	tt::return_list(std::vector<float>{ s.x, s.y, s.z });
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