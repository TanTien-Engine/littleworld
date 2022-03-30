#include "wrap_ArchGen.h"
#include "modules/render/Render.h"
#include "modules/script/TransHelper.h"

#include <unirender/Device.h>
#include <unirender/VertexBuffer.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexArray.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <polymesh3/Polytope.h>

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

struct Vertex
{
	sm::vec3 pos;
	sm::vec2 texcoord;
};

void w_GeometryTools_build_mesh()
{
	std::vector<std::shared_ptr<pm3::Polytope>> polys;
	tt::list_to_foreigns(1, polys);

	std::shared_ptr<pm3::TextureMapping> uv_map = nullptr;
	if (ves_toforeign(2)) {
		uv_map = ((tt::Proxy<pm3::TextureMapping>*)(ves_toforeign(2)))->obj;
	}

	std::vector<Vertex> vertices;
	std::vector<unsigned short> indices;

	size_t start_idx = 0;
	for (auto& poly : polys)
	{
		auto& points = poly->Points();
		for (auto& p : points)
		{
			Vertex vert;
			vert.pos = p->pos;
			if (uv_map) {
				vert.texcoord = uv_map->CalcTexCoords(p->pos, 1, 1);
			}
			vertices.push_back(vert);
		}

		auto& faces = poly->Faces();
		for (auto& f : faces) 
		{
			auto& polyline = f->border;
			if (polyline.size() > 2) {
				for (size_t i = 1, n = polyline.size() - 1; i < n; ++i) {
					indices.push_back(start_idx + polyline[0]);
					indices.push_back(start_idx + polyline[i]);
					indices.push_back(start_idx + polyline[i + 1]);
				}
			}
		}
		start_idx += points.size();
	}

	if (vertices.empty() || indices.empty()) {
		ves_set_nil(0);
		return;
	}

	auto dev = tt::Render::Instance()->Device();
	auto va = dev->CreateVertexArray();

	auto vbuf_sz = sizeof(Vertex) * vertices.size();
	auto vbuf = dev->CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
	vbuf->ReadFromMemory(vertices.data(), vbuf_sz, 0);
	va->SetVertexBuffer(vbuf);

	auto ibuf = dev->CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, 0);
	auto ibuf_sz = sizeof(unsigned short) * indices.size();
	ibuf->SetCount(indices.size());
	ibuf->Reserve(ibuf_sz);
	ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
	ibuf->SetDataType(ur::IndexBufferDataType::UnsignedShort);
	va->SetIndexBuffer(ibuf);

	std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs(2);
    // pos
    vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
        0, ur::ComponentDataType::Float, 3, 0, 20
    );
    // texcoord
    vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
        1, ur::ComponentDataType::Float, 2, 12, 20
    );
	va->SetVertexBufferAttrs(vbuf_attrs);

	ves_pop(ves_argnum());

	ves_pushnil();
	ves_import_class("render", "VertexArray");
	auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::VertexArray>));
	proxy->obj = va;
	ves_pop(1);
}

}

namespace archgen
{

VesselForeignMethodFn ArchGenBindMethod(const char* signature)
{
	if (strcmp(signature, "TextureMapping.clone()") == 0) return w_TextureMapping_clone;

	if (strcmp(signature, "static GeometryTools.build_mesh(_,_)") == 0) return w_GeometryTools_build_mesh;

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