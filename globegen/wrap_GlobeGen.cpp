#include "wrap_GlobeGen.h"
#include "ShapeBatching.h"
#include "VTexBuilder.h"
#include "VirtualTexture.h"

#include "modules/render/Render.h"
#include "modules/script/Proxy.h"
#include "modules/script/TransHelper.h"
#include "modules/graphics/Graphics.h"

#include <unirender/Device.h>
#include <unirender/VertexArray.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <geoshape/Shape3D.h>

namespace
{

void w_ShapeBatching_allocate()
{
    auto st = std::make_shared<globegen::ShapeBatching>();

    auto proxy = (tt::Proxy<globegen::ShapeBatching>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<globegen::ShapeBatching>));
    proxy->obj = st;
}

int w_ShapeBatching_finalize(void* data)
{
    auto proxy = (tt::Proxy<globegen::ShapeBatching>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<globegen::ShapeBatching>);
}

void w_ShapeBatching_add()
{
    auto sb = ((tt::Proxy<globegen::ShapeBatching>*)ves_toforeign(0))->obj;
    auto shape = ((tt::Proxy<gs::Shape3D>*)ves_toforeign(1))->obj;
    const uint32_t col = tt::list_to_abgr(2);
    sb->Add(shape, col);
}

void w_ShapeBatching_build_va()
{
    auto sb = ((tt::Proxy<globegen::ShapeBatching>*)ves_toforeign(0))->obj;
    auto& buf = sb->GetBuffer();

    auto dev = tt::Render::Instance()->Device();

    auto va = dev->CreateVertexArray();

    auto usage = ur::BufferUsageHint::StaticDraw;

    auto ibuf = dev->CreateIndexBuffer(usage, 0);
    int ibuf_sz = static_cast<int>(sizeof(unsigned short) * buf.indices.size());
    ibuf->SetCount(static_cast<int>(buf.indices.size()));
    ibuf->Reserve(ibuf_sz);
    ibuf->ReadFromMemory(buf.indices.data(), ibuf_sz, 0);
    ibuf->SetDataType(ur::IndexBufferDataType::UnsignedShort);
    va->SetIndexBuffer(ibuf);

    auto vbuf = dev->CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, 0);
    int vbuf_sz = static_cast<int>(sizeof(globegen::ShapeBatching::Vertex) * buf.vertices.size());
    vbuf->Reserve(vbuf_sz);
    vbuf->ReadFromMemory(buf.vertices.data(), vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);
    va->SetVertexBufferAttrs({
        std::make_shared<ur::VertexInputAttribute>(0, ur::ComponentDataType::Float,        3,  0, 16),		// position
		std::make_shared<ur::VertexInputAttribute>(1, ur::ComponentDataType::UnsignedByte, 4, 12, 16)		// color
    });

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("render", "VertexArray");
    auto proxy = (tt::Proxy<ur::VertexArray>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::VertexArray>));
    proxy->obj = va;
    ves_pop(1);
}

void w_VirtualTexture_allocate()
{
    auto filepath = ves_tostring(1);

    auto vtex = std::make_shared<globegen::VirtualTexture>(filepath);

    auto proxy = (tt::Proxy<globegen::VirtualTexture>*)ves_set_newforeign(0, 0, sizeof(tt::Proxy<globegen::VirtualTexture>));
    proxy->obj = vtex;
}

int w_VirtualTexture_finalize(void* data)
{
    auto proxy = (tt::Proxy<globegen::VirtualTexture>*)(data);
    proxy->~Proxy();
    return sizeof(tt::Proxy<globegen::VirtualTexture>);
}

void w_VirtualTexture_get_feedback_tex()
{
    auto vtex = ((tt::Proxy<globegen::VirtualTexture>*)ves_toforeign(0))->obj;

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("render", "Texture2D");
    auto proxy = (tt::Proxy<ur::Texture>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::Texture>));
    proxy->obj = vtex->GetFeedbackTex();
    ves_pop(1);
}

void w_VirtualTexture_get_atlas_tex()
{
    auto vtex = ((tt::Proxy<globegen::VirtualTexture>*)ves_toforeign(0))->obj;

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("render", "Texture2D");
    auto proxy = (tt::Proxy<ur::Texture>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::Texture>));
    proxy->obj = vtex->GetAtlasTex();
    ves_pop(1);
}

void w_VirtualTexture_get_page_table_tex()
{
    auto vtex = ((tt::Proxy<globegen::VirtualTexture>*)ves_toforeign(0))->obj;

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("render", "Texture2D");
    auto proxy = (tt::Proxy<ur::Texture>*)ves_set_newforeign(0, 1, sizeof(tt::Proxy<ur::Texture>));
    proxy->obj = vtex->GetTableTex();
    ves_pop(1);
}

void w_VirtualTexture_get_size()
{
    auto vtex = ((tt::Proxy<globegen::VirtualTexture>*)ves_toforeign(0))->obj;

    auto& sz = vtex->GetSize();

    std::vector<size_t> ret = { sz.vtex_width, sz.vtex_height, sz.tile_size, sz.border_size };
    tt::return_list(ret);
}

void w_VirtualTexture_update()
{
    auto vtex = ((tt::Proxy<globegen::VirtualTexture>*)ves_toforeign(0))->obj;

    sm::mat4* vp_mat = (sm::mat4*)ves_toforeign(1);;

    auto w = tt::Graphics::Instance()->GetWidth();
    auto h = tt::Graphics::Instance()->GetHeight();
    vtex->Update(*vp_mat, { w, h });
}

void w_GlobeTools_build_vtex()
{
    auto src = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
    auto dst = ves_tostring(2);
    auto vtex_sz   = (size_t)ves_tonumber(3);
    auto tile_sz   = (size_t)ves_tonumber(4);
    auto border_sz = (size_t)ves_tonumber(5);
    globegen::VTexBuilder::FromTexture(src, dst, vtex_sz, tile_sz, border_sz);
}

}

namespace globegen
{

VesselForeignMethodFn GlobeGenBindMethod(const char* signature)
{
    if (strcmp(signature, "ShapeBatching.add(_,_)") == 0) return w_ShapeBatching_add;
    if (strcmp(signature, "ShapeBatching.build_va()") == 0) return w_ShapeBatching_build_va;

    if (strcmp(signature, "VirtualTexture.get_feedback_tex()") == 0) return w_VirtualTexture_get_feedback_tex;
    if (strcmp(signature, "VirtualTexture.get_atlas_tex()") == 0) return w_VirtualTexture_get_atlas_tex;
    if (strcmp(signature, "VirtualTexture.get_page_table_tex()") == 0) return w_VirtualTexture_get_page_table_tex;
    if (strcmp(signature, "VirtualTexture.get_size()") == 0) return w_VirtualTexture_get_size;
    if (strcmp(signature, "VirtualTexture.update(_)") == 0) return w_VirtualTexture_update;

    if (strcmp(signature, "static GlobeTools.build_vtex(_,_,_,_,_)") == 0) return w_GlobeTools_build_vtex;

    return nullptr;
}

void GlobeGenBindClass(const char* class_name, VesselForeignClassMethods* methods)
{
    if (strcmp(class_name, "ShapeBatching") == 0)
    {
        methods->allocate = w_ShapeBatching_allocate;
        methods->finalize = w_ShapeBatching_finalize;
        return;
    }

    if (strcmp(class_name, "VirtualTexture") == 0)
    {
        methods->allocate = w_VirtualTexture_allocate;
        methods->finalize = w_VirtualTexture_finalize;
        return;
    }
}

}