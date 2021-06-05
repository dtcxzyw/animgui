// SPDX-License-Identifier: MIT

#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/common.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/style.hpp>

namespace animgui {
    ANIMGUI_API void text(canvas& canvas, std::pmr::string str) {
        primitive text =
            canvas_text{ vec2{ 0.0f, 0.0f }, std::move(str), canvas.style().default_font, canvas.style().font_color };
        const auto [w, h] = canvas.calculate_bounds(text);
        canvas.push_region(canvas.region_sub_uid(), bounds{ 0.0f, w, 0.0f, h });
        canvas.add_primitive("content"_id, std::move(text));
        canvas.pop_region();
    }
    static bool button_with_content(canvas& canvas, const std::function<void(animgui::canvas&)>& render_function) {
        const auto [region_idx, region_uid] = canvas.push_region(canvas.region_sub_uid(), bounds{ 0.0f, 0.0f, 0.0f, 0.0f });
        const auto hovered = canvas.region_hovered();
        const auto pressed = canvas.region_pressed(key_code::left_button);
        const auto res = pressed;
        auto [idx, uid] = canvas.add_primitive(
            "button_base"_id,
            button_base{ { 0.0f, 0.0f },
                         { 0.0f, 0.0f },
                         pressed ? button_status::pressed : (hovered ? button_status::hovered : button_status::normal) });
        const auto content_size = layout_row(canvas, row_alignment::left, render_function);
        auto&& inst = std::get<button_base>(std::get<primitive>(canvas.commands()[idx]));
        inst.content_size = content_size;
        const auto [w, h] = canvas.calculate_bounds(inst);
        std::get<push_region>(canvas.commands()[region_idx]).bounds = { 0.0f, w, 0.0f, h };
        canvas.pop_region();
        return res;
    }
    ANIMGUI_API bool button_label(canvas& canvas, std::pmr::string label) {
        return button_with_content(canvas, [&](animgui::canvas& sub_canvas) { text(sub_canvas, std::move(label)); });
    }
    ANIMGUI_API void image(canvas& canvas, texture_region image, const vec2 size, const color& factor) {
        primitive text = canvas_image{ { 0.0f, size.x, 0.0f, size.y }, std::move(image), factor };
        const auto [w, h] = canvas.calculate_bounds(text);
        canvas.push_region(canvas.region_sub_uid(), bounds{ 0.0f, w, 0.0f, h });
        canvas.add_primitive("content"_id, std::move(text));
        canvas.pop_region();
    }
    ANIMGUI_API bool button_image(canvas& canvas, texture_region image, vec2 size, const color& factor) {
        return button_with_content(
            canvas, [&](animgui::canvas& sub_canvas) { animgui::image(sub_canvas, std::move(image), size, factor); });
    }

}  // namespace animgui
