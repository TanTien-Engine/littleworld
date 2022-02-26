#pragma once

#include <primitive/Bitmap.h>

#include <memory>

namespace globegen
{

class ImageTools
{
public:
	template <typename T> 
	static std::shared_ptr<prim::Bitmap<T>> Cropping(const prim::Bitmap<T>& img, size_t x, size_t y,
		size_t w, size_t h, bool trim);

	template <typename T>
	static void Split(std::vector<std::shared_ptr<prim::Bitmap<T>>>& dst,
		const prim::Bitmap<T>& src, size_t tile_w, size_t tile_h, bool trim);

}; // ImageTools

}

#include "ImageTools.inl"