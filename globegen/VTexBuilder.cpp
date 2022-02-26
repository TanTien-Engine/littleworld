#include "VTexBuilder.h"
#include "VTexInfo.h"

#include <modules/render/Render.h>

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/TextureDescription.h>
#include <unirender/DrawState.h>
#include <unirender/ShaderProgram.h>
#include <unirender/Uniform.h>
#include <unirender/Texture.h>
#include <unirender/TextureUtility.h>

#include <functional>
#include <fstream>

namespace
{

const char* copy_cs = R"(

#version 430

#define LOCAL_SIZE 32

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

layout(binding = 0, r16f) writeonly uniform image2D out_tex;

uniform sampler2D in_tex;

uniform vec4 src_region;	// x start, x end, y start, y end

uniform int border_sz;

void main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	ivec2 img_sz = imageSize(out_tex);
	vec2 uv = (pos - ivec2(border_sz)) / float(img_sz - ivec2(border_sz * 2));
	uv = clamp(uv, 0.0, 1.0);
    uv = src_region.xz + uv * (src_region.yw - src_region.xz);

    vec4 rgba = texture(in_tex, uv);

    imageStore(out_tex, pos, rgba);
}

)";

const char* copy2_cs = R"(

#version 430

#define LOCAL_SIZE 32

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

layout(binding = 0, r16f) writeonly uniform image2D out_tex;

uniform sampler2D in_tex0, in_tex1;

uniform vec4 src_region;	// x start, x end, y start, y end

uniform int border_sz;

void main()
{
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	ivec2 img_sz = imageSize(out_tex);
	vec2 uv = (pos - ivec2(border_sz)) / float(img_sz - ivec2(border_sz * 2));
	uv = clamp(uv, 0.0, 1.0);
    uv = src_region.xz + uv * (src_region.yw - src_region.xz);

	vec4 rgba;
	if (uv.x < 0.5) {
		uv.x *= 2;
		rgba = texture(in_tex0, uv);
	} else {
		uv.x = (uv.x - 0.5) * 2;
		rgba = texture(in_tex1, uv);
	}
    imageStore(out_tex, pos, rgba);
}

)";

}

namespace globegen
{

void VTexBuilder::FromTexture(const std::shared_ptr<ur::Texture>& src_tex, const char* dst_path,
	                          size_t vtex_w, size_t vtex_h, size_t tile_sz, size_t border_sz)
{
	std::fstream file;
	file.open(dst_path, std::ios::out | std::ios::binary);

	VTexInfo header;
	header.vtex_width  = vtex_w;
	header.vtex_height = vtex_h;
	header.tile_size   = tile_sz;
	header.border_size = border_sz;
	header.channels    = 1;
	header.bytes       = 2;
	file.write(reinterpret_cast<const char*>(&header), sizeof(header));

	auto dev = tt::Render::Instance()->Device();
	auto ctx = tt::Render::Instance()->Context();

	auto prog = dev->CreateShaderProgram(copy_cs);

	auto i_border_sz = (int)border_sz;
	auto u_border_sz = prog->QueryUniform("border_sz");
	u_border_sz->SetValue(&i_border_sz, 1);

	int in_tex = prog->QueryTexSlot("in_tex");
	ctx->SetTexture(in_tex, src_tex);

	size_t tile_sz_with_border = tile_sz + border_sz * 2;

	ur::TextureDescription desc;
	desc.width = static_cast<int>(tile_sz_with_border);
	desc.height = static_cast<int>(tile_sz_with_border);
	desc.format = ur::TextureFormat::R16F;
	auto tile_tex = dev->CreateTexture(desc);

	int out_tex = prog->QueryImgSlot("out_tex");
	ctx->SetImage(out_tex, tile_tex, ur::AccessType::WriteOnly);

	std::function copy2tmp = [&](int x, int y, int tile_w, int tile_h)
	{
		dev->PushDebugGroup("Copy");

		ur::DrawState ds;

		ds.program = prog;

		auto u_region = prog->QueryUniform("src_region");
		float region[4] = {
			static_cast<float>(x) / tile_w,
			static_cast<float>(x + 1) / tile_w,
			static_cast<float>(y) / tile_h,
			static_cast<float>(y + 1) / tile_h
		};
		u_region->SetValue(region, 4);

		ctx->Compute(ds, tile_tex->GetWidth() / 32 + 1, tile_tex->GetHeight() / 32 + 1, 1);

		ctx->SetMemoryBarrier({ ur::BarrierType::ShaderImageAccess });

		dev->PopDebugGroup();
	};

	auto tile_data_sz = ur::TextureUtility::RequiredSizeInBytes(tile_tex->GetWidth(), tile_tex->GetHeight(), tile_tex->GetFormat(), 4);
	uint8_t* tile_data = new uint8_t[tile_data_sz];

//	size_t tile_num = std::min(vtex_w, vtex_h) / tile_sz;
	size_t tile_w = vtex_w / tile_sz;
	size_t tile_h = vtex_h / tile_sz;

	//const int mip_count = std::log2(tile_num) + 1;
	while (tile_w > 0 && tile_h > 0)
	{
		for (int y = 0; y < tile_h; ++y) {
			for (int x = 0; x < tile_w; ++x) {
				copy2tmp(x, y, tile_w, tile_h);

				tile_tex->WriteToMemory(tile_data);
				file.write(reinterpret_cast<const char*>(tile_data), tile_data_sz);
			}
		}
		tile_w /= 2;
		tile_h /= 2;
	}

	delete[] tile_data;

	file.close();
}

void VTexBuilder::FromTexture(const std::shared_ptr<ur::Texture>& src_tex_left,
	                          const std::shared_ptr<ur::Texture>& src_tex_right,
	                          const char* dst_path, size_t vtex_w, size_t vtex_h, size_t tile_sz, size_t border_sz)
{
	std::fstream file;
	file.open(dst_path, std::ios::out | std::ios::binary);

	VTexInfo header;
	header.vtex_width  = vtex_w;
	header.vtex_height = vtex_h;
	header.tile_size   = tile_sz;
	header.border_size = border_sz;
	header.channels    = 1;
	header.bytes       = 2;
	file.write(reinterpret_cast<const char*>(&header), sizeof(header));

	auto dev = tt::Render::Instance()->Device();
	auto ctx = tt::Render::Instance()->Context();

	auto prog = dev->CreateShaderProgram(copy2_cs);

	auto i_border_sz = (int)border_sz;
	auto u_border_sz = prog->QueryUniform("border_sz");
	u_border_sz->SetValue(&i_border_sz, 1);

	int in_tex0 = prog->QueryTexSlot("in_tex0");
	ctx->SetTexture(in_tex0, src_tex_left);
	int in_tex1 = prog->QueryTexSlot("in_tex1");
	ctx->SetTexture(in_tex1, src_tex_right);

	size_t tile_sz_with_border = tile_sz + border_sz * 2;

	ur::TextureDescription desc;
	desc.width = static_cast<int>(tile_sz_with_border);
	desc.height = static_cast<int>(tile_sz_with_border);
	desc.format = ur::TextureFormat::R16F;
	auto tile_tex = dev->CreateTexture(desc);

	int out_tex = prog->QueryImgSlot("out_tex");
	ctx->SetImage(out_tex, tile_tex, ur::AccessType::WriteOnly);

	std::function copy2tmp = [&](int x, int y, int tile_w, int tile_h)
	{
		dev->PushDebugGroup("Copy");

		ur::DrawState ds;

		ds.program = prog;

		auto u_region = prog->QueryUniform("src_region");
		float region[4] = {
			static_cast<float>(x) / tile_w,
			static_cast<float>(x + 1) / tile_w,
			static_cast<float>(y) / tile_h,
			static_cast<float>(y + 1) / tile_h
		};
		u_region->SetValue(region, 4);

		ctx->Compute(ds, tile_tex->GetWidth() / 32 + 1, tile_tex->GetHeight() / 32 + 1, 1);

		ctx->SetMemoryBarrier({ ur::BarrierType::ShaderImageAccess });

		dev->PopDebugGroup();
	};

	auto tile_data_sz = ur::TextureUtility::RequiredSizeInBytes(tile_tex->GetWidth(), tile_tex->GetHeight(), tile_tex->GetFormat(), 4);
	uint8_t* tile_data = new uint8_t[tile_data_sz];

//	size_t tile_num = std::min(vtex_w, vtex_h) / tile_sz;
	size_t tile_w = vtex_w / tile_sz;
	size_t tile_h = vtex_h / tile_sz;

	//const int mip_count = std::log2(tile_num) + 1;
	while (tile_w > 0 && tile_h > 0)
	{
		for (int y = 0; y < tile_h; ++y) {
			for (int x = 0; x < tile_w; ++x) {
				copy2tmp(x, y, tile_w, tile_h);

				tile_tex->WriteToMemory(tile_data);
				file.write(reinterpret_cast<const char*>(tile_data), tile_data_sz);
			}
		}
		tile_w /= 2;
		tile_h /= 2;
	}

	delete[] tile_data;

	file.close();
}

}