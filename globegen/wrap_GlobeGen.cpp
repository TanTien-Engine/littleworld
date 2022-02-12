#include "wrap_GlobeGen.h"
#include "ShapeBatching.h"
#include "modules/render/Render.h"
#include "modules/script/Proxy.h"
#include "modules/script/TransHelper.h"

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

}

namespace globegen
{

VesselForeignMethodFn GlobeGenBindMethod(const char* signature)
{
    if (strcmp(signature, "ShapeBatching.add(_,_)") == 0) return w_ShapeBatching_add;
    if (strcmp(signature, "ShapeBatching.build_va()") == 0) return w_ShapeBatching_build_va;

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
}

}