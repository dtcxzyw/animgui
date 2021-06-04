// SPDX-License-Identifier: MIT

#pragma once
#include "common.hpp"
#include <memory>

namespace animgui {
    class font;

    struct style {
        std::shared_ptr<font> fallback_font;
        uint32_t fallback_codepoint;
        std::shared_ptr<font> default_font;

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
