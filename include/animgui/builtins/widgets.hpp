// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>

namespace animgui {
    struct texture_region;
    class canvas;
    ANIMGUI_API bool selected(canvas& parent, identifier id);
    ANIMGUI_API bool clicked(canvas& parent, identifier id, bool pressed, bool focused);

    ANIMGUI_API void text(canvas& parent, std::pmr::string str);
    ANIMGUI_API void image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor);
    ANIMGUI_API bool button_label(canvas& parent, std::pmr::string label);
    ANIMGUI_API bool button_image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor);
    ANIMGUI_API void property(canvas& parent, int32_t& val, int32_t min, int32_t max, int32_t step, float smooth_step);
    ANIMGUI_API void property(canvas& parent, float& val, float min, float max, float step, float smooth_step);
    ANIMGUI_API void slider(canvas& parent, float width, float min_handle_width, int32_t& val, int32_t min, int32_t max);
    ANIMGUI_API void slider(canvas& parent, float width, float handle_width, float& val, float min, float max);
    ANIMGUI_API void checkbox(canvas& parent, std::pmr::string label, bool& state);
    ANIMGUI_API void switch_(canvas& parent, bool& state);
    enum class text_edit_status { inactive, active, committed };
    ANIMGUI_API text_edit_status text_edit(canvas& parent, float glyph_width, std::pmr::string& str,
                               std::optional<std::pmr::string> placeholder = std::nullopt);
    ANIMGUI_API void radio_button(canvas& parent, const std::pmr::vector<std::pmr::string>& labels, size_t& index);
    ANIMGUI_API void color_edit(canvas& parent, color_rgba& color);
    ANIMGUI_API void progressbar(canvas& parent, float width, float progress, std::optional<std::pmr::string> label);
    // TODO: badge & tooltip
}  // namespace animgui
