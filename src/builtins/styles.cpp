// SPDX-License-Identifier: MIT

#include <animgui/builtins/styles.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/style.hpp>

namespace animgui {
    void set_classic_style(context& context) {
        auto&& style = context.global_style();
        style.spacing = { 8.0f, 8.0f };
        style.rounding = 0.0f;
        style.bounds_edge_width = 3.0f;
        style.panel_bounds_edge_width = 5.0f;
        style.padding = { 8.0f, 8.0f };

        style.background = light_alpha(1.0f);
        style.panel_background = light_alpha(0.98f);

        style.text = { dark_alpha(0.87f), dark_alpha(0.54f), dark_alpha(0.38f), dark_alpha(0.38f) };
        style.action = { dark_alpha(0.54f), dark_alpha(0.04f), dark_alpha(0.08f), dark_alpha(0.26f) };
        style.primary = { 0x7986cb_html_rgb, 0x3f51b5_html_rgb, 0x303f9f_html_rgb, light_alpha(1.0f) };
        style.secondary = { 0xff4081_html_rgb, 0xf50057_html_rgb, 0xc51162_html_rgb, light_alpha(1.0f) };
    }
}  // namespace animgui
