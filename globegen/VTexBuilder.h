#pragma once

#include <memory>

namespace ur { class Texture; }

namespace globegen
{

class VTexBuilder
{
public:
	static void FromTexture(const std::shared_ptr<ur::Texture>& src_tex,
		const char* dst_path, size_t vtex_w, size_t vtex_h, size_t tile_sz, size_t border_sz);

	static void FromTexture(const std::shared_ptr<ur::Texture>& src_tex_left,
		const std::shared_ptr<ur::Texture>& src_tex_right,
		const char* dst_path, size_t vtex_w, size_t vtex_h, size_t tile_sz, size_t border_sz);

	static void PrepareDem15(const char* src_dir, const char* dst_path);
	static void FromTiles(const char* src_dir, int tile_num_x, int tile_num_y,
		const char* dst_path, size_t vtex_w, size_t vtex_h, size_t tile_sz, size_t border_sz);

}; // VTexBuilder

}