#include "VTexBuilder.h"
#include "VTexInfo.h"
#include "ImageTools.h"

#include <modules/render/Render.h>

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/TextureDescription.h>
#include <unirender/DrawState.h>
#include <unirender/ShaderProgram.h>
#include <unirender/Uniform.h>
#include <unirender/Texture.h>
#include <unirender/TextureUtility.h>
#include <gimg/gimg_import.h>
#include <gimg/gimg_export.h>
#include <gimg/gimg_typedef.h>
#include <SM_Vector.h>

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

typedef prim::Bitmap<uint16_t> Image;

std::string calc_tile_filepath(const char* dir_path, const sm::ivec2& pos)
{
	auto filename = std::to_string(pos.y) + "_" + std::to_string(pos.x) + ".tif";
	auto filepath = std::string(dir_path) + "\\" + filename;
	return filepath;
}

std::shared_ptr<Image> load_image(const char* dir_path, const sm::ivec2& pos)
{
	auto filepath = calc_tile_filepath(dir_path, pos);

	int width = 0, height = 0;
	int format = 0;
	uint16_t* pixels = (uint16_t*)gimg_import(filepath.c_str(), &width, &height, &format);
	assert(format == GPF_R16);

	auto img = std::make_shared<Image>(width, height, 1);
	memcpy(img->GetPixels(), pixels, width * height * 2);
	free(pixels);

	return img;
}

std::shared_ptr<ur::Texture> load_texture(const char* filepath)
{
	int width = 0, height = 0;
	int format = 0;
	uint16_t* pixels = (uint16_t*)gimg_import(filepath, &width, &height, &format);
	assert(format == GPF_R16);

	ur::TextureFormat tf;
	if (format == GPF_R16) {
		tf = ur::TextureFormat::R16;
	} else if (format == GPF_RED) {
		tf = ur::TextureFormat::RED;
	}
	size_t buf_sz = ur::TextureUtility::RequiredSizeInBytes(width, height, tf, 1);
	auto ret = tt::Render::Instance()->Device()->CreateTexture(width, height, tf, pixels, buf_sz, false);
	free(pixels);
	return ret;
}

void copy2tile(const std::shared_ptr<ur::ShaderProgram>& prog, 
	           const std::shared_ptr<ur::Texture>& tile_tex,
	           int x, int y, int tile_w, int tile_h)
{
	auto dev = tt::Render::Instance()->Device();
	auto ctx = tt::Render::Instance()->Context();

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
}

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
				copy2tile(prog, tile_tex, x, y, tile_w, tile_h);

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
				copy2tile(prog, tile_tex, x, y, tile_w, tile_h);

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

void VTexBuilder::PrepareDem15(const char* src_dir, const char* dst_path)
{
	auto img3to2 = [&](const sm::ivec2 img3[3]) -> std::pair<std::shared_ptr<Image>, std::shared_ptr<Image>>
	{
		auto img0 = load_image(src_dir, img3[0]);
		auto img1 = load_image(src_dir, img3[1]);
		auto img2 = load_image(src_dir, img3[2]);

		std::vector<std::shared_ptr<Image>> mid_imgs;
		size_t h_width = static_cast<size_t>(std::ceilf(img1->Width() * 0.5f));
		globegen::ImageTools::Split<uint16_t>(mid_imgs, *img1, h_width, img1->Height(), false);

		auto left  = globegen::ImageTools::MergeHori<uint16_t>(*img0, *mid_imgs[0]);
		auto right = globegen::ImageTools::MergeHori<uint16_t>(*mid_imgs[1], *img2);

		return std::make_pair(left, right);
	};

	auto img1to4 = [&](const std::shared_ptr<Image>& img) -> std::vector<std::shared_ptr<Image>>
	{
		std::vector<std::shared_ptr<Image>> ret;
		size_t h_width = static_cast<size_t>(std::ceilf(img->Width() * 0.5f));
		size_t h_height = static_cast<size_t>(std::ceilf(img->Height() * 0.5f));
		globegen::ImageTools::Split<uint16_t>(ret, *img, h_width, h_height, false);
		return ret;
	};

	for (int grid_y = 0; grid_y < 4; ++grid_y)
	{
		for (int grid_x = 0; grid_x < 2; ++grid_x)
		{
			sm::ivec2 img3[3] = { sm::ivec2(grid_x * 3, grid_y), sm::ivec2(grid_x * 3 + 1, grid_y), sm::ivec2(grid_x * 3 + 2, grid_y) };
			auto img2 = img3to2(img3);

			auto img_l = img2.first;
			auto img_r = img2.second;

			auto img_l_4 = img1to4(img2.first);
			for (int i = 0; i < 4; ++i) 
			{
				int x = i % 2;
				int y = 1 - i / 2;
				int dst_x = grid_x * 4 + x;
				int dst_y = grid_y * 2 + y;
				auto filepath = calc_tile_filepath("D:/projects/gis/dem15/new", sm::ivec2(dst_x, dst_y));

				auto img = img_l_4[i];
				gimg_export(filepath.c_str(), (uint8_t*)img->GetPixels(), img->Width(), img->Height(), GPF_R16, 1);
			}

			auto img_r_4 = img1to4(img2.second);
			for (int i = 0; i < 4; ++i)
			{
				int x = i % 2;
				int y = 1 - i / 2;
				int dst_x = grid_x * 4 + 2 + x;
				int dst_y = grid_y * 2 + y;
				auto filepath = calc_tile_filepath("D:/projects/gis/dem15/new", sm::ivec2(dst_x, dst_y));

				auto img = img_r_4[i];
				gimg_export(filepath.c_str(), (uint8_t*)img->GetPixels(), img->Width(), img->Height(), GPF_R16, 1);
			}
		}
	}
}

void VTexBuilder::MergeDem15(const char* src_dir, const char* dst_path)
{
	const int tile_sz = 2048;
	const int tile_num = 8;

	auto dev = tt::Render::Instance()->Device();
	auto ctx = tt::Render::Instance()->Context();

	ur::TextureDescription desc_tile;
	desc_tile.width = tile_sz;
	desc_tile.height = tile_sz;
	desc_tile.format = ur::TextureFormat::R16F;
	auto tile_tex = dev->CreateTexture(desc_tile);

	ur::TextureDescription desc_tot;
	desc_tot.width = tile_sz * tile_num;
	desc_tot.height = tile_sz * tile_num;
	desc_tot.format = ur::TextureFormat::R16F;
	auto tot_tex = dev->CreateTexture(desc_tot);

	auto prog = dev->CreateShaderProgram(copy_cs);
	int out_tex = prog->QueryImgSlot("out_tex");
	ctx->SetImage(out_tex, tile_tex, ur::AccessType::WriteOnly);

	auto full = std::make_shared<prim::Bitmap<uint16_t>>(tot_tex->GetWidth(), tot_tex->GetHeight(), 1);
	auto tile = std::make_shared<prim::Bitmap<uint16_t>>(tile_tex->GetWidth(), tile_tex->GetHeight(), 1);
	for (int y = 0; y < tile_num; ++y) {
		for (int x = 0; x < tile_num; ++x) {
			auto filepath = calc_tile_filepath(src_dir, sm::ivec2(x, y));
			auto tex = load_texture(filepath.c_str());
			int in_tex = prog->QueryTexSlot("in_tex");
			ctx->SetTexture(in_tex, tex);

			copy2tile(prog, tile_tex, 0, 0, 1, 1);
			tile_tex->WriteToMemory(tile->GetPixels());

			ImageTools::Copy(full, tile, sm::ivec2(x * tile_sz, y * tile_sz));
		}
	}

	gimg_export(dst_path, (uint8_t*)full->GetPixels(), full->Width(), full->Height(), GPF_R16, 0);
}

void VTexBuilder::FromTiles(const char* src_dir, int tile_num_x, int tile_num_y,
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

	auto prog = dev->CreateShaderProgram(copy_cs);

	auto i_border_sz = (int)border_sz;
	auto u_border_sz = prog->QueryUniform("border_sz");
	u_border_sz->SetValue(&i_border_sz, 1);

	size_t tile_sz_with_border = tile_sz + border_sz * 2;

	ur::TextureDescription desc;
	desc.width = static_cast<int>(tile_sz_with_border);
	desc.height = static_cast<int>(tile_sz_with_border);
	desc.format = ur::TextureFormat::R16F;
	auto tile_tex = dev->CreateTexture(desc);

	int out_tex = prog->QueryImgSlot("out_tex");
	ctx->SetImage(out_tex, tile_tex, ur::AccessType::WriteOnly);

	auto tile_data_sz = ur::TextureUtility::RequiredSizeInBytes(tile_tex->GetWidth(), tile_tex->GetHeight(), tile_tex->GetFormat(), 4);
	uint8_t* tile_data = new uint8_t[tile_data_sz];

	for (int tile_x = 0; tile_x < tile_num_x; ++tile_x)
	{
		for (int tile_y = 0; tile_y < tile_num_y; ++tile_y)
		{
			auto filepath = calc_tile_filepath(src_dir, sm::ivec2(tile_x, tile_y));
			printf("pack %s\n", filepath.c_str());

			auto tex = load_texture(filepath.c_str());
			int in_tex = prog->QueryTexSlot("in_tex");
			ctx->SetTexture(in_tex, tex);

			uint64_t file_offset = sizeof(header);

			size_t tile_w = vtex_w / tile_sz / tile_num_x;
			size_t tile_h = vtex_h / tile_sz / tile_num_y;
			while (tile_w > 0 && tile_h > 0)
			{
				for (int y = 0; y < tile_h; ++y) {
					for (int x = 0; x < tile_w; ++x) {
						copy2tile(prog, tile_tex, x, y, tile_w, tile_h);
						tile_tex->WriteToMemory(tile_data);

						int xx = tile_w * tile_x + x;
						int yy = tile_h * tile_y + y;
						uint64_t offset = (yy * (tile_w * tile_num_x) + xx) * tile_data_sz;

						file.seekg(file_offset + offset, std::ios_base::beg);
						file.write(reinterpret_cast<const char*>(tile_data), tile_data_sz);
					}
				}

				file_offset += tile_data_sz * (tile_w * tile_num_x) * (tile_h * tile_num_y);

				tile_w /= 2;
				tile_h /= 2;
			}
		}
	}

	auto tex = load_texture("D:/projects/gis/dem15/merge.tif");
	int in_tex = prog->QueryTexSlot("in_tex");
	ctx->SetTexture(in_tex, tex);

	size_t tile_w = tile_num_x / 2;
	size_t tile_h = tile_num_y / 2;

	uint64_t file_offset = sizeof(header);
	size_t tile_w_tot = vtex_w / tile_sz;
	size_t tile_h_tot = vtex_h / tile_sz;
	while (tile_w_tot > 0 && tile_h_tot > 0)
	{
		file_offset += tile_data_sz * tile_w_tot * tile_h_tot;

		tile_w_tot /= 2;
		tile_h_tot /= 2;
		if (tile_w_tot == tile_w && tile_h_tot == tile_h) {
			break;
		}
	}
	file.seekg(file_offset, std::ios_base::beg);

	while (tile_w > 0 && tile_h > 0)
	{
		for (int y = 0; y < tile_h; ++y) {
			for (int x = 0; x < tile_w; ++x) {
				copy2tile(prog, tile_tex, x, y, tile_w, tile_h);
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