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

    LEDPanel(int width = 32, int height = 8);

    void resize(int width, int height);
    void clear(uint32_t color = 0);
    void set_corner(Corner corner) { corner_ = corner; }
    Corner corner() const { return corner_; }
    int width() const { return width_; }
    int height() const { return height_; }

    void set_pixel(int x, int y, uint32_t color);
    void fill_rect(int x, int y, int width, int height, uint32_t color);
    void draw_text(const std::string& text, int x, uint32_t color, int character_spacing = 0);
    int text_width(const std::string& text, int character_spacing = 0) const;

    // Return pixels in physical serial order.  Mapping is serpentine today;
    // keeping it here makes other wiring layouts a contained future addition.
    const std::vector<uint32_t>& pixels() const { return pixels_; }

private:
    int pixel_index(int x, int y) const;
    int width_;
    int height_;
    Corner corner_ = Corner::TopLeft;
    std::vector<uint32_t> pixels_;
};
