#pragma once

#include <cstdint>
#include <string>
#include <vector>

// A logical, coordinate-addressed LED panel.  It deliberately knows nothing
// about PIO or a particular animation, so banner-style applications can share
// it with the Magic 8-ball mode.
class LEDPanel {
public:
    enum class Corner : int { TopLeft, TopRight, BottomLeft, BottomRight };
    enum class FontSpacing : int { Fixed = 0, Proportional = 1 };

    struct GlyphMetrics {
        uint8_t left = 0;
        uint8_t width = 8;
    };

    LEDPanel(int width = 32, int height = 8);

    void resize(int width, int height);
    void clear(uint32_t color = 0);
    void set_corner(Corner corner) { corner_ = corner; }
    Corner corner() const { return corner_; }
    int width() const { return width_; }
    int height() const { return height_; }

    void set_font_spacing(FontSpacing spacing) { font_spacing_ = spacing; }
    FontSpacing font_spacing() const { return font_spacing_; }

    void set_pixel(int x, int y, uint32_t color);
    void fill_rect(int x, int y, int width, int height, uint32_t color);
    void draw_text(const std::string& text, int x, uint32_t color, int character_spacing = -1);
    int text_width(const std::string& text, int character_spacing = -1) const;

    // Return pixels in physical serial order.  Mapping is serpentine today;
    // keeping it here makes other wiring layouts a contained future addition.
    const std::vector<uint32_t>& pixels() const { return pixels_; }

    static const GlyphMetrics& glyph_metrics(char c);

private:
    int pixel_index(int x, int y) const;
    static void init_glyph_metrics();

    int width_;
    int height_;
    Corner corner_ = Corner::TopLeft;
    FontSpacing font_spacing_ = FontSpacing::Fixed;
    std::vector<uint32_t> pixels_;
};
