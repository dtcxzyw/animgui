// SPDX-License-Identifier: MIT

#pragma once
#include "common.hpp"
#include <memory>

namespace animgui {
    class font;

    struct style {
        std::shared_ptr<font> font;

        color panel_background_color;
        color background_color;
        color normal_color;
        color highlight_color;
        color disabled_color;
        color font_color;

        vec2 padding;
        vec2 spacing;
        float rounding;
    };
}  // namespace animgui
