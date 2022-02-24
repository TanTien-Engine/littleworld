#include "VTexBuilder.h"

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

}

namespace globegen
{

void VTexBuilder::FromTexture(const std::shared_ptr<ur::Texture>& src_tex, const char* dst_path,
	                          size_t vtex_sz, size_t tile_sz, size_t border_sz)
{
	std::fstream file;
	file.open(dst_path, std::ios::out | std::ios::binary);

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

	std::function copy2tmp = [&](int x, int y, size_t tile_num)
	{
		dev->PushDebugGroup("Copy");

		ur::DrawState ds;

		ds.program = prog;

		auto u_region = prog->QueryUniform("src_region");
		float region[4] = {
			static_cast<float>(x) / tile_num,
			static_cast<float>(x + 1) / tile_num,
			static_cast<float>(y) / tile_num,
			static_cast<float>(y + 1) / tile_num
		};
		u_region->SetValue(region, 4);

		ctx->Compute(ds, tile_tex->GetWidth() / 32 + 1, tile_tex->GetHeight() / 32 + 1, 1);

		ctx->SetMemoryBarrier({ ur::BarrierType::ShaderImageAccess });

		dev->PopDebugGroup();
	};

	auto tile_data_sz = ur::TextureUtility::RequiredSizeInBytes(tile_tex->GetWidth(), tile_tex->GetHeight(), tile_tex->GetFormat(), 4);
	uint8_t* tile_data = new uint8_t[tile_data_sz];

	size_t tile_num = vtex_sz / tile_sz;
	//const int mip_count = std::log2(tile_num) + 1;
	while (tile_num > 0)
	{
		for (int y = 0; y < tile_num; ++y) {
			for (int x = 0; x < tile_num; ++x) {
				copy2tmp(x, y, tile_num);

				tile_tex->WriteToMemory(tile_data);
				file.write(reinterpret_cast<const char*>(tile_data), tile_data_sz);
			}
		}
		tile_num /= 2;
	}

	delete[] tile_data;

	file.close();
}

}