// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>

namespace animgui {
    enum class text_edit_status;
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

    class ANIMGUI_API checkbox_modifier {
        canvas_stroke_rect& m_ref_bound;
        canvas_fill_rect* m_ptr_select;
        canvas_text& m_ref_label;
        bool m_state;

    public:
        checkbox_modifier(canvas& parent, size_t idx_bound, size_t idx_select, size_t idx_label, bool state);
        checkbox_modifier& bound_color(const color_rgba& color);
        checkbox_modifier& bound_size(float size);
        checkbox_modifier& select_color(const color_rgba& color);
        checkbox_modifier& font(std::shared_ptr<font> font);
        explicit operator bool() const;
    };

    class ANIMGUI_API switch_modifier {
        canvas_fill_rect& m_ref_base;
        canvas_fill_rect& m_ref_handle;
        canvas_text& m_ref_label;
        canvas_stroke_rect& m_ref_bounds;
        bool m_focused;
        bool m_state;

    public:
        switch_modifier(canvas& parent, size_t idx_base, size_t idx_handle, size_t idx_label, size_t idx_bounds, bool focused, bool state);
        switch_modifier& base_color(const color_rgba& color);
        switch_modifier& set_handle_style(const style& new_style);
        switch_modifier& label_font(std::shared_ptr<font> font);
        switch_modifier& label_color(const color_rgba& color);
        switch_modifier& set_bounds_style(const style& new_style);
        explicit operator bool() const;
    };

    class ANIMGUI_API radio_button_modifier {
        std::vector<button_base*> m_ref_button_bases;
        std::vector<canvas_text*> m_ref_labels;
        size_t m_count;
        size_t m_index;

    public:
        radio_button_modifier(canvas& parent, const std::vector<size_t>& idx_button_bases, const std::vector<size_t>& idx_labels, size_t index);
        radio_button_modifier& set_style(const style& new_style);
        radio_button_modifier& label_font(std::shared_ptr<font> font);
    };

    class ANIMGUI_API progressbar_modifier {
        canvas_fill_rect& m_ref_base;
        canvas_fill_rect& m_ref_progress;
        canvas_stroke_rect& m_ref_bounds;
        canvas_text* m_ref_label;
        
    public:
        progressbar_modifier(canvas& parent, size_t idx_base, size_t idx_progress, size_t idx_bounds, bool has_label, size_t idx_label);
        progressbar_modifier& base_color(const color_rgba& color);
        progressbar_modifier& progress_color(const color_rgba& color);
        progressbar_modifier& set_bounds_style(const style& new_style);
        progressbar_modifier& label_font(std::shared_ptr<font> font);
        progressbar_modifier& label_color(const color_rgba& color);
    };

    class ANIMGUI_API text_edit_modifier {
        canvas_stroke_rect& m_ref_background;
        canvas_fill_rect* m_ref_cursor_rect;
        canvas_line* m_ref_cursor_line;
        canvas_fill_rect* m_ref_select;
        canvas_text& m_ref_content;
        bool m_active;
        text_edit_status m_status;
        bool m_str_empty;

    public:
        text_edit_modifier(canvas& parent, size_t idx_background, bool edit, bool selecting, bool cursor_show, bool rect_cursor,
                           size_t idx_cursor_rect, size_t idx_cursor_line, size_t idx_select, size_t idx_content, bool active,
                           text_edit_status status, bool m_str_empty);
        text_edit_modifier& set_style(const style& new_style);
        text_edit_modifier& cursor_line_size(float size);
        text_edit_modifier& content_font(std::shared_ptr<font> font);
        explicit operator text_edit_status() const;
    };

    ANIMGUI_API text_modifier text(canvas& parent, std::pmr::string str);
    ANIMGUI_API image_modifier image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor);
    ANIMGUI_API button_label_modifier button_label(canvas& parent, std::pmr::string label);
    ANIMGUI_API button_image_modifier button_image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor);
    ANIMGUI_API void property(canvas& parent, int32_t& val, int32_t min, int32_t max, int32_t step, float smooth_step);
    ANIMGUI_API void property(canvas& parent, float& val, float min, float max, float step, float smooth_step);
    ANIMGUI_API slider_modifier slider(canvas& parent, float width, float min_handle_width, int32_t& val, int32_t min, int32_t max);
    ANIMGUI_API slider_modifier slider(canvas& parent, float width, float handle_width, float& val, float min, float max);
    ANIMGUI_API checkbox_modifier checkbox(canvas& parent, std::pmr::string label, bool& state);
    ANIMGUI_API switch_modifier switch_(canvas& parent, bool& state);
    enum class text_edit_status { inactive, active, committed };
    ANIMGUI_API text_edit_modifier text_edit(canvas& parent, float glyph_width, std::pmr::string& str,
                                           std::optional<std::pmr::string> placeholder = std::nullopt);
    ANIMGUI_API radio_button_modifier radio_button(canvas& parent, const std::pmr::vector<std::pmr::string>& labels, size_t& index);
    ANIMGUI_API void color_edit(canvas& parent, color_rgba& color);
    ANIMGUI_API progressbar_modifier progressbar(canvas& parent, float width, float progress, std::optional<std::pmr::string> label);
    // TODO: badge & tooltip
}  // namespace animgui
