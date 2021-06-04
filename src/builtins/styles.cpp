// SPDX-License-Identifier: MIT

#include <animgui/builtins/styles.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/style.hpp>

namespace animgui {
    void set_classic_style(context& context) {
        auto&& style = context.style();
        style.spacing = { 5.0f, 5.0f };
        style.rounding = 0.0f;
        style.padding = { 3.0f, 3.0f };

        style.font_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        style.background_color = { 0.3f, 0.3f, 0.3f, 1.0f };
        style.normal_color = { 0.25f, 0.25f, 0.25f, 1.0f };
        style.highlight_color = { 0.5f, 0.5f, 0.5f, 1.0f };
        style.disabled_color = { 0.2f, 0.2f, 0.2f, 1.0f };
    }
}  // namespace animgui
