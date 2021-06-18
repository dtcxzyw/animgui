// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>

namespace animgui {
    struct texture_region;
    class canvas;
    ANIMGUI_API bool selected(canvas& canvas, uid id);
    ANIMGUI_API bool clicked(canvas& canvas, uid id, bool pressed, bool hovered);

    ANIMGUI_API void text(canvas& canvas, std::pmr::string str);
    ANIMGUI_API void image(canvas& canvas, texture_region image, vec2 size, const color& factor);
    ANIMGUI_API bool button_label(canvas& canvas, std::pmr::string label);
    ANIMGUI_API bool button_image(canvas& canvas, texture_region image, vec2 size, const color& factor);
    ANIMGUI_API void property(canvas& canvas, int32_t& val, int32_t min, int32_t max, int32_t step, float smooth_step);
    ANIMGUI_API void property(canvas& canvas, float& val, float min, float max, float step, float smooth_step);
    ANIMGUI_API void slider(canvas& canvas, float width, float handle_width, int32_t& val, int32_t min, int32_t max);
    ANIMGUI_API void slider(canvas& canvas, float width, float handle_width, float& val, float min, float max);
    ANIMGUI_API void spinner(canvas& canvas, int32_t& val, int32_t min, int32_t max, int32_t step);
    ANIMGUI_API void spinner(canvas& canvas, float& val, float min, float max, float step);
    ANIMGUI_API void checkbox(canvas& canvas, std::pmr::string label, bool& state);
    ANIMGUI_API void switch_(canvas& canvas, std::pmr::string label, bool& state);
    ANIMGUI_API void text_edit(canvas& canvas, float glyph_width, std::pmr::string& str,
                               std::optional<std::pmr::string> placeholder = std::nullopt);
    ANIMGUI_API void radio_button(canvas& canvas, const std::pmr::vector<std::pmr::string>& labels, size_t& index);
    ANIMGUI_API void color_edit(canvas& canvas, color& color);
    ANIMGUI_API void progressbar(canvas& canvas, float width, float progress, std::optional<std::pmr::string> label);
    // TODO: badge & tooltip
}  // namespace animgui
