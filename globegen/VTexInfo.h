#pragma once

namespace globegen
{

struct VTexInfo
{
    size_t vtex_width  = 0;
    size_t vtex_height = 0;

    size_t tile_size   = 0;
    size_t border_size = 0;

    size_t channels = 0;
    size_t bytes    = 0;

	int PageSize() const { return tile_size + 2 * border_size; }
	int PageTableWidth() const { return vtex_width / tile_size; }
    int PageTableHeight() const { return vtex_height / tile_size; }

}; // VTexInfo

}