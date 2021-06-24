// SPDX-License-Identifier: MIT

#pragma once
#include "common.hpp"
#include <memory>

namespace animgui {
    class font;

    struct style {
        std::shared_ptr<font> default_font;

        color_rgba panel_background_color;
        color_rgba background_color;
        color_rgba normal_color;
        color_rgba highlight_color;
        color_rgba disabled_color;
        color_rgba font_color;
        color_rgba selected_color;

        vec2 padding;
        vec2 spacing;
        float rounding;
        float bounds_edge_width;
        float panel_bounds_edge_width;
    };
}  // namespace animgui
