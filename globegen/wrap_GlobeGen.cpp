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

const unsigned char g_bit_rev_table[] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, 0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1, 0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD, 0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

const unsigned short g_bit_rev_masks[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0X0100, 0X0300, 0X0700, 0X0F00, 0X1F00, 0X3F00, 0X7F00, 0XFF00 };

unsigned long reverse_bits(unsigned long codeword, unsigned char maxLength)
{
    if (maxLength <= 8)
    {
        codeword = g_bit_rev_table[codeword << (8 - maxLength)];
    }
    else
    {
        unsigned char sc = maxLength - 8; // shift count in bits and index into masks array

        if (maxLength <= 16)
        {
            codeword = (g_bit_rev_table[codeword & 0X00FF] << sc)
                | g_bit_rev_table[codeword >> sc];
        }
        else if (maxLength & 1) // if maxLength is 17, 19, 21, or 23
        {
            codeword = (g_bit_rev_table[codeword & 0X00FF] << sc)
                | g_bit_rev_table[codeword >> sc] | (g_bit_rev_table[(codeword & g_bit_rev_masks[sc]) >> (sc - 8)] << 8);
        }
        else // if maxlength is 18, 20, 22, or 24
        {
            codeword = (g_bit_rev_table[codeword & 0X00FF] << sc)
                | g_bit_rev_table[codeword >> sc]
                | (g_bit_rev_table[(codeword & g_bit_rev_masks[sc]) >> (sc >> 1)] << (sc >> 1));
        }
    }

    return codeword;
}

void w_GlobeTools_generate_bit_reversed_indices()
{
    int n = (int)ves_tonumber(1);

    std::vector<int32_t> indices;
    indices.resize(n);

    int32_t num_bits = int(log(n) / log(2));

    for (int i = 0; i < n; i++) {
        indices[i] = reverse_bits(i, num_bits);
    }

    tt::return_list(indices);
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

    if (strcmp(signature, "static GlobeTools.generate_bit_reversed_indices(_)") == 0) return w_GlobeTools_generate_bit_reversed_indices;

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