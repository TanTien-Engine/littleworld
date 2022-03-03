#include "wrap_GlobeGen.h"
#include "ShapeBatching.h"
#include "VTexBuilder.h"
#include "VirtualTexture.h"
#include "ImageTools.h"

#include "modules/render/Render.h"
#include "modules/script/Proxy.h"
#include "modules/script/TransHelper.h"
#include "modules/graphics/Graphics.h"
#include "modules/filesystem/Filesystem.h"

#include <unirender/Device.h>
#include <unirender/VertexArray.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/TextureUtility.h>
#include <geoshape/Shape3D.h>
#include <gimg/gimg_import.h>
#include <gimg/gimg_typedef.h>

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
    auto sphere = ves_toboolean(2);

    globegen::VTexGeoType geo_type = globegen::VTexGeoType::Plane;
    if (sphere) {
        geo_type = globegen::VTexGeoType::Spherre;
    }

    auto vtex = std::make_shared<globegen::VirtualTexture>(filepath, geo_type);

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

    sm::mat4* mvp_mat = (sm::mat4*)ves_toforeign(1);;

    auto w = tt::Graphics::Instance()->GetWidth();
    auto h = tt::Graphics::Instance()->GetHeight();
    vtex->Update(*mvp_mat, { w, h });
}

void w_GlobeTools_build_vtex()
{
    auto src = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
    auto dst = ves_tostring(2);
    auto vtex_w    = (size_t)ves_tonumber(3);
    auto vtex_h    = (size_t)ves_tonumber(4);
    auto tile_sz   = (size_t)ves_tonumber(5);
    auto border_sz = (size_t)ves_tonumber(6);
    globegen::VTexBuilder::FromTexture(src, dst, vtex_w, vtex_h, tile_sz, border_sz);
}

void w_GlobeTools_build_vtex2()
{
    auto src0 = ((tt::Proxy<ur::Texture>*)ves_toforeign(1))->obj;
    auto src1 = ((tt::Proxy<ur::Texture>*)ves_toforeign(2))->obj;
    auto dst = ves_tostring(3);
    auto vtex_w    = (size_t)ves_tonumber(4);
    auto vtex_h    = (size_t)ves_tonumber(5);
    auto tile_sz   = (size_t)ves_tonumber(6);
    auto border_sz = (size_t)ves_tonumber(7);
    globegen::VTexBuilder::FromTexture(src0, src1, dst, vtex_w, vtex_h, tile_sz, border_sz);
}

void w_GlobeTools_prepare_dem15()
{
    auto src = ves_tostring(1);
    auto dst = ves_tostring(2);
    globegen::VTexBuilder::PrepareDem15(src, dst);
}

void w_GlobeTools_merge_dem15()
{
    auto src = ves_tostring(1);
    auto dst = ves_tostring(2);
    globegen::VTexBuilder::MergeDem15(src, dst);
}

void w_GlobeTools_build_vtex_tiles()
{
    auto src        = ves_tostring(1);
    auto tile_num_x = (size_t)ves_tonumber(2);
    auto tile_num_y = (size_t)ves_tonumber(3);
    auto dst        = ves_tostring(4);
    auto vtex_w     = (size_t)ves_tonumber(5);
    auto vtex_h     = (size_t)ves_tonumber(6);
    auto tile_sz    = (size_t)ves_tonumber(7);
    auto border_sz  = (size_t)ves_tonumber(8);
    globegen::VTexBuilder::FromTiles(src, tile_num_x, tile_num_y, dst, vtex_w, vtex_h, tile_sz, border_sz);
}

void w_GlobeTools_split_image()
{
    auto filepath = ves_tostring(1);
    size_t num_x = (size_t)ves_tonumber(2);
    size_t num_y = (size_t)ves_tonumber(3);

    int width = 0, height = 0;
    int format = 0;
    uint8_t* pixels = nullptr;
    if (tt::Filesystem::IsExists(filepath)) {
        pixels = gimg_import(filepath, &width, &height, &format);
    } else {
        std::string path = tt::Filesystem::Instance()->GetAssetBaseDir() + "/" + filepath;
        pixels = gimg_import(path.c_str(), &width, &height, &format);
    }

    assert(format == GPF_LUMINANCE);
    if (format != GPF_LUMINANCE) {
        return;
    }

    auto src_img = std::make_shared<prim::Bitmap<uint8_t>>(width, height, 1);
    memcpy(src_img->GetPixels(), pixels, sizeof(uint8_t) * width * height);
    free(pixels);
    std::vector<std::shared_ptr<prim::Bitmap<uint8_t>>> dst_imgs;
    globegen::ImageTools::Split<uint8_t>(dst_imgs, *src_img, width / num_x, height / num_y, false);

    ves_pop(ves_argnum());

    const int num = (int)(dst_imgs.size());
    ves_newlist(num);
    for (int i = 0; i < num; ++i)
    {
        auto img = dst_imgs[i];
        auto tf = ur::TextureFormat::RED;
        size_t buf_sz = ur::TextureUtility::RequiredSizeInBytes(img->Width(), img->Height(), tf, 4);
        auto tex = tt::Render::Instance()->Device()->CreateTexture(img->Width(), img->Height(), tf, img->GetPixels(), buf_sz);

        ves_pushnil();
        ves_import_class("render", "Texture2D");
        auto proxy = (tt::Proxy<ur::Texture>*)ves_set_newforeign(1, 2, sizeof(tt::Proxy<ur::Texture>));
        proxy->obj = tex;
        ves_pop(1);
        ves_seti(-2, i);
        ves_pop(1);
    }
}

static const float Rg = 6360.0;

sm::vec3 calc_cam_pos(float lon, float lat, float theta, float phi, float d)
{
    double co = cos(lon);
    double so = sin(lon);
    double ca = cos(lat);
    double sa = sin(lat);
    sm::vec3 po = sm::vec3(co * ca, so * ca, sa) * Rg;
    sm::vec3 px = sm::vec3(-so, co, 0);
    sm::vec3 py = sm::vec3(-co * sa, -so * sa, ca);
    sm::vec3 pz = sm::vec3(co * ca, so * ca, sa);

    double ct = cos(theta);
    double st = sin(theta);
    double cp = cos(phi);
    double sp = sin(phi);
    sm::vec3 cz = px * sp * st - py * cp * st + pz * ct;
    sm::vec3 position = po + cz * d;

    return position;
}

void w_GlobeTools_cam_view_mat()
{
    float lon = ves_tonumber(1);
    float lat = ves_tonumber(2);
    float theta = ves_tonumber(3);
    float phi = ves_tonumber(4);
    float d = ves_tonumber(5);

    double co = cos(lon);
    double so = sin(lon);
    double ca = cos(lat);
    double sa = sin(lat);
    sm::vec3 po = sm::vec3(co * ca, so * ca, sa) * Rg;
    sm::vec3 px = sm::vec3(-so, co, 0);
    sm::vec3 py = sm::vec3(-co * sa, -so * sa, ca);
    sm::vec3 pz = sm::vec3(co * ca, so * ca, sa);

    double ct = cos(theta);
    double st = sin(theta);
    double cp = cos(phi);
    double sp = sin(phi);
    sm::vec3 cx = px * cp + py * sp;
    sm::vec3 cy = -px * sp * ct + py * cp * ct + pz * st;
    sm::vec3 cz = px * sp * st - py * cp * st + pz * ct;
    sm::vec3 position = po + cz * d;

    if (position.Length() < Rg + 0.01) 
    {
        auto length = sqrt(position.x * position.x + position.y * position.y + position.z * position.z);
        auto invLength = (Rg + 0.01) / length;
        position.x *= invLength;
        position.y *= invLength;
        position.z *= invLength;
    }

    float m[16] = {
        cx.x, cx.y, cx.z, 0,
        cy.x, cy.y, cy.z, 0,
        cz.x, cz.y, cz.z, 0,
        0, 0, 0, 1
    };

    sm::mat4 view;
    memcpy(view.c, m, sizeof(m));
    
    view = view * sm::mat4::Translated(-position.x, -position.y, -position.z);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("maths", "Matrix44");
    sm::mat4* mat = (sm::mat4*)ves_set_newforeign(0, 1, sizeof(sm::mat4));
    *mat = view;
    ves_pop(1);
}

void w_GlobeTools_cam_proj_mat()
{
    float lon = ves_tonumber(1);
    float lat = ves_tonumber(2);
    float theta = ves_tonumber(3);
    float phi = ves_tonumber(4);
    float d = ves_tonumber(5);

    auto position = calc_cam_pos(lon, lat, theta, phi, d);

    float width  = tt::Graphics::Instance()->GetWidth();
    float height = tt::Graphics::Instance()->GetHeight();

    float h = position.Length() - Rg;
    float vfov = 2.0 * atan(float(height) / float(width) * tan(80.0 / 180 * SM_PI / 2.0)) / SM_PI * 180;
    sm::mat4 proj = sm::mat4::Perspective(vfov, float(width) / float(height), 0.1 * h, 1e5 * h);

    ves_pop(ves_argnum());

    ves_pushnil();
    ves_import_class("maths", "Matrix44");
    sm::mat4* mat = (sm::mat4*)ves_set_newforeign(0, 1, sizeof(sm::mat4));
    *mat = proj;
    ves_pop(1);
}

void w_GlobeTools_cam_position()
{
    float lon = ves_tonumber(1);
    float lat = ves_tonumber(2);
    float theta = ves_tonumber(3);
    float phi = ves_tonumber(4);
    float d = ves_tonumber(5);

    auto p = calc_cam_pos(lon, lat, theta, phi, d);

    tt::return_list(std::vector<float>{ p.x, p.y, p.z });
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

    if (strcmp(signature, "static GlobeTools.build_vtex(_,_,_,_,_,_)") == 0) return w_GlobeTools_build_vtex;
    if (strcmp(signature, "static GlobeTools.build_vtex2(_,_,_,_,_,_,_)") == 0) return w_GlobeTools_build_vtex2;
    if (strcmp(signature, "static GlobeTools.prepare_dem15(_,_)") == 0) return w_GlobeTools_prepare_dem15;
    if (strcmp(signature, "static GlobeTools.merge_dem15(_,_)") == 0) return w_GlobeTools_merge_dem15;
    if (strcmp(signature, "static GlobeTools.build_vtex_tiles(_,_,_,_,_,_,_,_)") == 0) return w_GlobeTools_build_vtex_tiles;
    if (strcmp(signature, "static GlobeTools.split_image(_,_,_)") == 0) return w_GlobeTools_split_image;
    if (strcmp(signature, "static GlobeTools.cam_view_mat(_,_,_,_,_)") == 0) return w_GlobeTools_cam_view_mat;
    if (strcmp(signature, "static GlobeTools.cam_proj_mat(_,_,_,_,_)") == 0) return w_GlobeTools_cam_proj_mat;
    if (strcmp(signature, "static GlobeTools.cam_position(_,_,_,_,_)") == 0) return w_GlobeTools_cam_position;

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