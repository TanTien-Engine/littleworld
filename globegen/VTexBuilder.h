#pragma once

#include <memory>

namespace ur { class Texture; }

namespace globegen
{

class VTexBuilder
{
public:
	static void FromTexture(const std::shared_ptr<ur::Texture>& src_tex,
		const char* dst_path, size_t vtex_sz, size_t tile_sz);

}; // VTexBuilder

}