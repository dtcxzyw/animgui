// SPDX-License-Identifier: MIT

#pragma once
#include <string>

namespace animgui {
    struct texture_region;
    class canvas;
    void text(canvas& canvas, std::pmr::string str);
    bool button_label(canvas& canvas, std::pmr::string label);
    bool button_image(canvas& canvas, const texture_region& image);
    // void modal(canvas& parent, const std::function<void(canvas&)>& render_function);
}  // namespace animgui
