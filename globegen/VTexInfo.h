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

    size_t PageSize() const { return tile_size + 2 * border_size; }
    size_t PageTableWidth() const { return vtex_width / tile_size; }
    size_t PageTableHeight() const { return vtex_height / tile_size; }

}; // VTexInfo

}