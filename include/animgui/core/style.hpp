// SPDX-License-Identifier: MIT

#pragma once
#include "common.hpp"
#include <memory>

namespace animgui {
    class font;

    struct semantic_color_group final {
        color_rgba light;
        color_rgba main;
        color_rgba dark;
        color_rgba text;
    };

    struct text_color_group final {
        color_rgba primary;
        color_rgba secondary;
        color_rgba disabled;
        color_rgba hint;
    };

    struct action_color_group final {
        color_rgba active;
        color_rgba hover;
        color_rgba selected;
        color_rgba disabled;
    };

    struct style final {
        std::shared_ptr<font> default_font;

        color_rgba background;
        color_rgba panel_background;

        text_color_group text;
        action_color_group action;

        semantic_color_group primary;
        semantic_color_group secondary;

        /*
        semantic_color_group info;
        semantic_color_group warning;
        semantic_color_group error;
        semantic_color_group success;
        */

        vec2 padding;
        vec2 spacing;
        float rounding;
        float bounds_edge_width;
        float panel_bounds_edge_width;
    };

    constexpr color_rgba operator""_html_rgb(const unsigned long long int code) {
        return color_rgba{ static_cast<float>((code >> 16) & 255) / 255.0f, static_cast<float>((code >> 8) & 255) / 255.0f,
                           static_cast<float>(code & 255) / 255.0f, 1.0f };
    }
    constexpr color_rgba dark_alpha(const float alpha) {
        return color_rgba{ 0.0f, 0.0f, 0.0f, alpha };
    }
    constexpr color_rgba light_alpha(const float alpha) {
        return color_rgba{ 1.0f, 1.0f, 1.0f, alpha };
    }
}  // namespace animgui
