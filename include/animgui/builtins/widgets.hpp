// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>

namespace animgui {
    struct texture_region;
    class canvas;
    void text(canvas& canvas, std::pmr::string str);
    void image(canvas& canvas, texture_region image, vec2 size, const color& factor);
    bool button_label(canvas& canvas, std::pmr::string label);
    bool button_image(canvas& canvas, texture_region image, vec2 size, const color& factor);
    // void modal(canvas& parent, const std::function<void(canvas&)>& render_function);
}  // namespace animgui
