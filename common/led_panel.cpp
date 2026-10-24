#include "led_panel.hpp"

#include <algorithm>

#include "font8x8_basic.h"

LEDPanel::LEDPanel(int width, int height):
    width_(width), height_(height), pixels_(width * height, 0)
{
}

void LEDPanel::resize(int width, int height)
{
    width_ = width;
    height_ = height;
    pixels_.assign(width * height, 0);
}

void LEDPanel::clear(uint32_t color)
{
    std::fill(pixels_.begin(), pixels_.end(), color);
}

int LEDPanel::pixel_index(int x, int y) const
{
    // Count rows and columns from the selected physical LED-0 corner, then
    // apply the presently supported horizontal serpentine layout.  In every
    // corner setting the logical pixel at that corner maps to serial LED 0.
    const int row_from_start = (corner_ == Corner::BottomLeft || corner_ == Corner::BottomRight) ? height_ - 1 - y : y;
    const int col_from_start = (corner_ == Corner::TopRight || corner_ == Corner::BottomRight) ? width_ - 1 - x : x;
    return row_from_start * width_ + ((row_from_start & 1) ? width_ - 1 - col_from_start : col_from_start);
}

void LEDPanel::set_pixel(int x, int y, uint32_t color)
{
    if(x >= 0 && x < width_ && y >= 0 && y < height_) pixels_[pixel_index(x, y)] = color;
}

void LEDPanel::fill_rect(int x, int y, int width, int height, uint32_t color)
{
    for(int iy = y; iy < y + height; ++iy)
        for(int ix = x; ix < x + width; ++ix) set_pixel(ix, iy, color);
}

void LEDPanel::draw_text(const std::string& text, int x, uint32_t color)
{
    for(unsigned ichar = 0; ichar < text.size(); ++ichar) {
        const unsigned char c = static_cast<unsigned char>(text[ichar]);
        if(c >= 128) continue;
        for(int y = 0; y < 8 && y < height_; ++y) {
            const unsigned char bits = static_cast<unsigned char>(font8x8_basic[c][y]);
            for(int bit = 0; bit < 8; ++bit)
                if(bits & (1u << bit)) set_pixel(x + int(ichar) * 9 + bit, y, color);
        }
    }
}

int LEDPanel::text_width(const std::string& text) const
{
    return text.empty() ? 0 : int(text.size()) * 9 - 1;
}
