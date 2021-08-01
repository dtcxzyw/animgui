// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>

namespace animgui {
    struct texture_region;
    class canvas;
    struct style;

    ANIMGUI_API bool selected(canvas& parent, identifier id);
    ANIMGUI_API bool clicked(canvas& parent, identifier id, bool pressed, bool focused);

    class ANIMGUI_API text_modifier {
        canvas_text& m_ref;

    public:
        text_modifier(canvas& parent, size_t idx);
        text_modifier& font(std::shared_ptr<font> font);
        text_modifier& text_color(const color_rgba& color);
    };


    class ANIMGUI_API button_label_modifier {
        canvas_fill_rect& m_ref_base;
        canvas_text& m_ref_text;
        bool m_clicked;
        bool m_pressed;
        bool m_focused;

    public:
        button_label_modifier(canvas& parent, size_t idx_base, size_t idx_text, bool clicked, bool pressed, bool focused);
        button_label_modifier& font(std::shared_ptr<font> font);
        button_label_modifier& set_style(const style& new_style);
        explicit operator bool() const;
    };


    class ANIMGUI_API image_modifier {
        canvas_image& m_ref;

    public:
        image_modifier(canvas& parent, size_t idx);
        image_modifier& factor(const color_rgba& factor);
    };

    class ANIMGUI_API button_image_modifier {
        canvas_fill_rect& m_ref_base;
        image_modifier m_img_modifier;
        bool m_clicked;
        bool m_pressed;
        bool m_focused;

    public:
        button_image_modifier(canvas& parent, size_t idx_base, image_modifier img_modifier, bool clicked, bool pressed, bool focused);
        button_image_modifier& set_style(const style& new_style);
        button_image_modifier& factor(const color_rgba& color);
        explicit operator bool() const;
    };

    class ANIMGUI_API slider_modifier {
        canvas_fill_rect& m_ref_base;
        canvas_fill_rect& m_ref_handle;
        bool m_focused;

    public:
        slider_modifier(canvas& parent, size_t idx_base, size_t idx_handle, bool focus);
        slider_modifier& set_style(const style& new_style);
    };

    ANIMGUI_API text_modifier text(canvas& parent, std::pmr::string str);
    ANIMGUI_API image_modifier image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor);
    ANIMGUI_API button_label_modifier button_label(canvas& parent, std::pmr::string label);
    ANIMGUI_API button_image_modifier button_image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor);
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
