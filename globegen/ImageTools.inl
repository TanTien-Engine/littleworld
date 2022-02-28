#pragma once

namespace globegen
{

template <typename T>
std::shared_ptr<prim::Bitmap<T>> ImageTools::Cropping(const prim::Bitmap<T>& img, size_t sub_x, size_t sub_y, size_t sub_w, size_t sub_h, bool trim)
{
    auto w = img.Width();
    auto h = img.Height();
    auto c = img.Channels();
    if (sub_x >= static_cast<int>(w) ||
        sub_y >= static_cast<int>(h)) {
        return nullptr;
    }

    auto src_pixels = img.GetPixels();

    size_t min_x = sub_x;
    size_t min_y = sub_y;
    size_t max_x = std::min(min_x + sub_w, w);
    size_t max_y = std::min(min_y + sub_h, h);

    size_t dw = 0, dh = 0;
    if (trim) {
        dw = max_x - min_x;
        dh = max_y - min_y;
    } else {
        dw = sub_w;
        dh = sub_h;
    }
    auto ret = std::make_shared<prim::Bitmap<T>>(dw, dh, c);
    auto dst_pixels = ret->GetPixels();

    for (size_t y = min_y; y < max_y; ++y) {
        for (size_t x = min_x; x < max_x; ++x) {
            for (size_t i = 0; i < c; ++i) {
                auto s_idx = (y * w + x) * c + i;
                auto d_idx = ((y - min_y) * dw + x - min_x) * c + i;
                dst_pixels[d_idx] = src_pixels[s_idx];
            }
        }
    }

    return ret;
}

template <typename T>
void ImageTools::Split(std::vector<std::shared_ptr<prim::Bitmap<T>>>& dst,
                       const prim::Bitmap<T>& src, size_t tile_w, size_t tile_h, bool trim)
{
    size_t w = src.Width();
    size_t h = src.Height();
    if (w == 0 || h == 0) {
        return;
    }

    size_t sw = tile_w;
    size_t sh = tile_h;
    if (sw == 0 || sh == 0) {
        return;
    }

    for (size_t y = 0; y < h; y += sh) {
        for (size_t x = 0; x < w; x += sw) {
            auto img = Cropping(src, x, y, sw, sh, trim);
            if (!img) {
                continue;
            }
            dst.push_back(img);
        }
    }
}

template <typename T>
std::shared_ptr<prim::Bitmap<T>> ImageTools::MergeHori(const prim::Bitmap<T>& left, const prim::Bitmap<T>& right)
{
    auto w0 = left.Width();
    auto h0 = left.Height();
    auto c0 = left.Channels();

    auto w1 = right.Width();
    auto h1 = right.Height();
    auto c1 = right.Channels();

    if (h0 != h1 || c0 != c1) {
        return nullptr;
    }

    auto ret = std::make_shared<prim::Bitmap<T>>(w0 + w1, h0, c0);
    auto dst_pixels = ret->GetPixels();

    auto l_pixels = left.GetPixels();
    for (size_t y = 0; y < h0; ++y) {
        for (size_t x = 0; x < w0; ++x) {
            for (size_t i = 0; i < c0; ++i) {
                auto s_idx = (y * w0 + x) * c0 + i;
                auto d_idx = (y * (w0 + w1) + x) * c0 + i;
                dst_pixels[d_idx] = l_pixels[s_idx];
            }
        }
    }

    auto r_pixels = right.GetPixels();
    for (size_t y = 0; y < h0; ++y) {
        for (size_t x = 0; x < w1; ++x) {
            for (size_t i = 0; i < c0; ++i) {
                auto s_idx = (y * w1 + x) * c0 + i;
                auto d_idx = (y * (w0 + w1) + x + w0) * c0 + i;
                dst_pixels[d_idx] = r_pixels[s_idx];
            }
        }
    }

    return ret;
}

template <typename T>
void ImageTools::Copy(const std::shared_ptr<prim::Bitmap<T>>& dst, 
                      const std::shared_ptr<prim::Bitmap<T>>& src,
                      const sm::ivec2& pos)
{
    if (dst->Channels() != src->Channels()) {
        return;
    }

    int dw = dst->Width();
    int dh = dst->Height();
    int sw = src->Width();
    int sh = src->Height();

    if (pos.x >= dw || pos.y >= dh) {
        return;
    }

    auto dp = dst->GetPixels();
    auto sp = src->GetPixels();

    auto c = dst->Channels();

    int xmin = pos.x;
    int ymin = pos.y;
    int xmax = std::min(xmin + sw, dw);
    int ymax = std::min(ymin + sh, dh);
    for (int y = ymin; y < ymax; ++y) {
        for (int x = xmin; x < xmax; ++x) {
            for (int z = 0; z < c; ++z) {
                uint64_t s = (y - ymin) * sw + (x - xmin);
                uint64_t d = y * dw + x;
                dp[d * c + z] = sp[s * c + z];
            }
        }
    }
}

}