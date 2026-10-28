#include "led_panel.hpp"

#include <algorithm>

#include "font8x8_basic.h"

namespace {
    LEDPanel::GlyphMetrics metrics[128];
    bool metrics_initialized = false;
}

void LEDPanel::init_glyph_metrics()
{
    if(metrics_initialized) return;
    for(int c = 0; c < 128; ++c) {
        uint8_t col_mask = 0;
        for(int y = 0; y < 8; ++y) {
            col_mask |= static_cast<uint8_t>(font8x8_basic[c][y]);
        }
        if(col_mask == 0) {
            metrics[c].left = 0;
            metrics[c].width = (c == ' ') ? 3 : 0;
        } else {
            uint8_t left = 0;
            while(left < 8 && !(col_mask & (1u << left))) ++left;
            uint8_t right = 7;
            while(right > 0 && !(col_mask & (1u << right))) --right;
            metrics[c].left = left;
            metrics[c].width = (right >= left) ? (right - left + 1) : 0;
        }
    }
    metrics_initialized = true;
}

const LEDPanel::GlyphMetrics& LEDPanel::glyph_metrics(char ch)
{
    init_glyph_metrics();
    unsigned char c = static_cast<unsigned char>(ch);
    static const GlyphMetrics empty{0, 0};
    return (c < 128) ? metrics[c] : empty;
}

LEDPanel::LEDPanel(int width, int height):
    width_(width), height_(height), pixels_(width * height, 0)
{
    init_glyph_metrics();
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
    // apply the presently supported vertical (column-major) serpentine layout.
    // In every corner setting the logical pixel at that corner maps to serial
    // LED 0: LED 0 is column 0 / row 0, LED 1 is column 0 / row 1, etc.
    const int row_from_start = (corner_ == Corner::BottomLeft || corner_ == Corner::BottomRight) ? height_ - 1 - y : y;
    const int col_from_start = (corner_ == Corner::TopRight || corner_ == Corner::BottomRight) ? width_ - 1 - x : x;
    return col_from_start * height_ + ((col_from_start & 1) ? height_ - 1 - row_from_start : row_from_start);
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

void LEDPanel::draw_text(const std::string& text, int x, uint32_t color, int character_spacing)
{
    init_glyph_metrics();
    const int spacing = (character_spacing >= 0) ? character_spacing :
                        ((font_spacing_ == FontSpacing::Proportional) ? 1 : 0);

    if(font_spacing_ == FontSpacing::Proportional) {
        int cursor_x = x;
        for(char ch : text) {
            unsigned char c = static_cast<unsigned char>(ch);
            if(c >= 128) continue;
            const GlyphMetrics& m = metrics[c];
            if(m.width > 0) {
                for(int y = 0; y < 8 && y < height_; ++y) {
                    const uint8_t bits = static_cast<uint8_t>(font8x8_basic[c][y]);
                    for(int bit = m.left; bit < m.left + m.width; ++bit) {
                        if(bits & (1u << bit)) {
                            set_pixel(cursor_x + (bit - m.left), y, color);
                        }
                    }
                }
                cursor_x += m.width + spacing;
            } else if(c == ' ') {
                cursor_x += 3 + spacing;
            }
        }
    } else {
        for(unsigned ichar = 0; ichar < text.size(); ++ichar) {
            const unsigned char c = static_cast<unsigned char>(text[ichar]);
            if(c >= 128) continue;
            for(int y = 0; y < 8 && y < height_; ++y) {
                const unsigned char bits = static_cast<unsigned char>(font8x8_basic[c][y]);
                for(int bit = 0; bit < 8; ++bit) {
                    if(bits & (1u << bit)) {
                        set_pixel(x + int(ichar) * (8 + spacing) + bit, y, color);
                    }
                }
            }
        }
    }
}

int LEDPanel::text_width(const std::string& text, int character_spacing) const
{
    if(text.empty()) return 0;
    init_glyph_metrics();
    const int spacing = (character_spacing >= 0) ? character_spacing :
                        ((font_spacing_ == FontSpacing::Proportional) ? 1 : 0);

    if(font_spacing_ == FontSpacing::Proportional) {
        int w = 0;
        for(char ch : text) {
            unsigned char c = static_cast<unsigned char>(ch);
            if(c >= 128) continue;
            const GlyphMetrics& m = metrics[c];
            int char_w = (c == ' ') ? 3 : m.width;
            w += char_w + spacing;
        }
        return std::max(0, w - spacing);
    } else {
        return int(text.size()) * (8 + spacing);
    }
}
