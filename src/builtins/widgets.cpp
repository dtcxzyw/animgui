// SPDX-License-Identifier: MIT

#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/common.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/style.hpp>
#include <cassert>
#include <utf8.h>

namespace animgui {
    ANIMGUI_API void text(canvas& canvas, std::pmr::string str) {
        primitive text = canvas_text{ vec2{ 0.0f, 0.0f }, std::move(str), canvas.style().font, canvas.style().font_color };
        const auto [w, h] = canvas.calculate_bounds(text);
        canvas.push_region(canvas.region_sub_uid(), bounds{ 0.0f, w, 0.0f, h });
        canvas.add_primitive("content"_id, std::move(text));
        canvas.pop_region();
    }
    ANIMGUI_API bool clicked(canvas& canvas, const uid id, const bool pressed, const bool hovered) {
        auto& last_pressed = canvas.storage<bool>(id);
        const auto res = last_pressed && !pressed && hovered;
        last_pressed = pressed;
        return res;
    }
    static bool button_with_content(canvas& canvas, const std::function<void(animgui::canvas&)>& render_function) {
        canvas.push_region(canvas.region_sub_uid());
        const auto hovered = canvas.region_hovered() || canvas.region_request_focus();
        const auto pressed = canvas.region_pressed(key_code::left_button);
        auto [idx, uid] = canvas.add_primitive(
            "button_base"_id,
            button_base{ { 0.0f, 0.0f },
                         { 0.0f, 0.0f },
                         pressed ? button_status::pressed : (hovered ? button_status::hovered : button_status::normal) });
        const auto res = clicked(canvas, uid, pressed, hovered);
        canvas.push_region(canvas.region_sub_uid());
        const auto content_size = layout_row(canvas, row_alignment::left, render_function);
        auto&& inst = std::get<button_base>(std::get<primitive>(canvas.commands()[idx]));
        inst.content_size = content_size;
        const auto [w, h] = canvas.calculate_bounds(inst);
        const auto padding_x = (w - content_size.x) / 2.0f;
        const auto padding_y = (h - content_size.y) / 2.0f;
        canvas.pop_region(bounds{ padding_x, padding_x + content_size.x, padding_y, padding_y + content_size.y });
        canvas.pop_region(bounds{ 0.0f, w, 0.0f, h });
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
    ANIMGUI_API bool selected(canvas& canvas, const uid id) {
        auto& [state, pressed] = canvas.storage<std::pair<bool, bool>>(mix(id, "selected"_id));
        if(canvas.region_hovered() && !pressed && canvas.region_pressed(key_code::left_button)) {
            state = true;
        } else if(!canvas.input_backend().get_key(key_code::left_button))
            state = false;
        pressed = canvas.input_backend().get_key(key_code::left_button);
        return state;
    }
    ANIMGUI_API void text_edit(canvas& canvas, const float glyph_width, std::pmr::string& str,
                               std::optional<std::pmr::string> placeholder) {
        auto&& style = canvas.style();
        const auto height = style.font->height() + 2.0f * style.padding.y;
        const auto width = style.font->standard_width() * glyph_width + 2.0f * style.padding.x;
        const bounds full_bounds{ 0.0f, width, 0.0f, height };

        const auto uid = canvas.push_region(canvas.region_sub_uid(), full_bounds).second;

        struct edit_state final {
            bool edit;
            bool override_mode;
            size_t pos_beg;
            size_t pos_end;
            float offset;
            edit_state() noexcept : edit{ false }, override_mode{ false }, pos_beg{ 0 }, pos_end{ 0 }, offset{ 0.0f } {}
        };

        auto&& state = canvas.storage<edit_state>(uid);
        auto&& [edit, override_mode, pos_beg, pos_end, offset] = state;

        if(selected(canvas, uid)) {
            // TODO: select
            edit = true;

            const auto pos = canvas.input_backend().get_cursor_pos().x;
            auto current_pos = canvas.region_offset().x + style.padding.x + offset;

            pos_beg = 0;
            auto iter = str.cbegin();
            glyph prev{ 0 };
            while(iter != str.cend()) {
                const auto cp = utf8::next(iter, str.cend());
                const auto glyph = style.font->to_glyph(cp);
                const auto distance = style.font->calculate_advance(glyph, prev);
                current_pos += distance;
                if(current_pos > pos)
                    break;
                prev = glyph;
                ++pos_beg;
            }

            pos_end = pos_beg;

        } else if(edit && canvas.input_backend().get_key(key_code::left_button)) {
            edit = false;
            offset = 0.0f;
            override_mode = false;
        }

        if(edit) {
            auto& input_backend = canvas.input_backend();
            std::pmr::string::const_iterator beg, end;

            const auto update_iterator = [&] {
                beg = str.cbegin();
                utf8::advance(beg, state.pos_beg, str.cend());
                end = beg;
                utf8::advance(end, state.pos_end - state.pos_beg, str.cend());
            };

            update_iterator();

            const auto delete_selected = [&] {
                if(state.pos_beg == state.pos_end)
                    return;

                str.erase(beg, end);
                state.pos_end = state.pos_beg;
                update_iterator();
            };
            const auto copy_selected = [&] {
                if(beg == end)
                    return;
                input_backend.set_clipboard_text(std::pmr::string{ beg, end, canvas.memory_resource() });
            };
            const auto insert_to = [&, override_ = override_mode](const std::string_view text, const bool select) {
                if(state.pos_beg != state.pos_end)
                    delete_selected();

                const auto size = static_cast<size_t>(utf8::distance(text.cbegin(), text.cend()));
                if(override_) {
                    auto iter = beg;
                    size_t cnt = 0;
                    while(iter != str.end() && cnt < size) {
                        utf8::next(iter, str.cend());
                        ++cnt;
                    }
                    str.erase(beg, iter);
                }

                str.insert(beg, text.cbegin(), text.cend());
                state.pos_end = state.pos_beg + size;
                if(!select)
                    state.pos_beg = state.pos_end;

                update_iterator();
            };

            if(input_backend.get_key_pulse(key_code::insert, false)) {
                override_mode = !override_mode;
            } else if(input_backend.get_key_pulse(key_code::back, true)) {
                if(pos_beg == pos_end && beg != str.cbegin()) {
                    utf8::prior(beg, str.cbegin());
                    --pos_beg;
                }
                delete_selected();
            } else if(input_backend.get_key_pulse(key_code::delete_, true)) {
                if(pos_beg == pos_end && end != str.cend()) {
                    utf8::next(end, str.cend());
                    ++pos_end;
                }
                delete_selected();
            } else if(input_backend.get_key_pulse(key_code::left, true)) {
                if(pos_beg == pos_end) {
                    if(pos_beg >= 1) {
                        pos_end = --pos_beg;
                        utf8::prior(beg, str.cbegin());
                        end = beg;
                    }
                } else {
                    pos_end = pos_beg;
                    end = beg;
                }
            } else if(input_backend.get_key_pulse(key_code::right, true)) {
                if(pos_beg == pos_end) {
                    if(end != str.cend()) {
                        pos_beg = ++pos_end;
                        utf8::next(end, str.cend());
                        beg = end;
                    }
                } else {
                    beg = end;
                    pos_beg = pos_end;
                }
            } else if(pos_beg != pos_end && input_backend.get_key_pulse(key_code::alpha_c, false) &&
                      input_backend.get_modifier_key(modifier_key::control)) {
                copy_selected();
            } else if(input_backend.get_key_pulse(key_code::alpha_v, false) &&
                      input_backend.get_modifier_key(modifier_key::control)) {
                insert_to(input_backend.get_clipboard_text(), true);
            } else if(pos_beg != pos_end && input_backend.get_key_pulse(key_code::alpha_x, false) &&
                      input_backend.get_modifier_key(modifier_key::control)) {
                copy_selected();
                delete_selected();
            } else if(input_backend.get_key_pulse(key_code::alpha_a, false) &&
                      input_backend.get_modifier_key(modifier_key::control)) {
                pos_beg = 0;
                pos_end = utf8::distance(str.cbegin(), str.cend());
                beg = str.cbegin();
                end = str.cend();
            } else {
                if(const auto seq = input_backend.get_input_characters(); seq.size()) {
                    std::pmr::string text{ canvas.memory_resource() };
                    text.resize(seq.size() * 4);
                    auto iter = text.begin();
                    for(auto cp : seq)
                        iter = utf8::append(cp, iter);
                    insert_to({ text.data(), static_cast<size_t>(iter - text.begin()) }, false);
                }
            }
        }

        auto active = edit | canvas.region_request_focus();  // TODO: focus

        if(canvas.region_hovered()) {
            canvas.input_backend().set_cursor(cursor::edit);
            active = true;
        }

        canvas.add_primitive(
            "background"_id,
            canvas_stroke_rect{ full_bounds, active ? style.highlight_color : style.normal_color, style.bounds_edge_width });
        if(edit) {
            float start_pos = style.padding.x + offset;
            float end_pos = style.padding.x + offset;
            glyph prev{ 0 };
            size_t current_pos = 0;
            auto iter = str.cbegin();
            while(iter != str.cend()) {
                const auto cp = utf8::next(iter, str.cend());
                const auto glyph = style.font->to_glyph(cp);
                const auto distance = style.font->calculate_advance(glyph, prev);
                if(override_mode && current_pos == pos_end) {
                    end_pos += distance;
                    break;
                }
                if(current_pos < pos_beg)
                    start_pos += distance;
                if(current_pos < pos_end)
                    end_pos += distance;
                prev = glyph;
                ++current_pos;
                if(current_pos == pos_end && !override_mode)
                    break;
            }

            if(pos_beg == pos_end) {

                if(start_pos < style.padding.x) {
                    offset += style.padding.x - start_pos;
                    start_pos = style.padding.x;
                } else if(const auto end = width - style.padding.x; end_pos > end) {
                    offset -= end_pos - end;
                    end_pos = end;
                }

                using clock = std::chrono::high_resolution_clock;
                // ReSharper disable once CppTooWideScope
                constexpr auto one_second = static_cast<uint64_t>(clock::period::den);

                if(static_cast<uint64_t>(clock::now().time_since_epoch().count()) % one_second > one_second / 2) {
                    if(override_mode && std::fabs(start_pos - end_pos) > 0.01f) {
                        canvas.add_primitive("cursor"_id,
                                             canvas_fill_rect{ bounds{ start_pos, end_pos, style.padding.y,
                                                                       style.padding.y + style.font->height() },
                                                               style.highlight_color });
                    } else
                        canvas.add_primitive("cursor"_id,
                                             canvas_line{ { start_pos, style.padding.y },
                                                          { start_pos, style.padding.y + style.font->height() },
                                                          style.highlight_color,
                                                          style.bounds_edge_width });
                }
            } else {
                canvas.add_primitive(
                    "selected_background"_id,
                    canvas_fill_rect{ bounds{ start_pos, end_pos, style.padding.y, style.padding.y + style.font->height() },
                                      style.selected_color });
            }
        }

        canvas.push_region(
            "text_region"_id,
            bounds{ style.padding.x, width - style.padding.x, style.padding.y, style.padding.y + style.font->height() });
        auto text = !edit && str.empty() ? std::move(placeholder).value_or("") : str;
        canvas.add_primitive(
            "content"_id,
            canvas_text{ { offset, 0.0f }, std::move(text), style.font, str.empty() ? style.disabled_color : style.font_color });
        canvas.pop_region();

        canvas.pop_region();
    }
    ANIMGUI_API void checkbox(canvas& canvas, std::pmr::string label, bool& state) {
        const auto id = canvas.push_region(canvas.region_sub_uid()).second;
        const auto hovered = canvas.region_hovered();

        if(clicked(canvas, id, canvas.region_pressed(key_code::left_button), hovered))
            state = !state;

        const auto active = canvas.region_request_focus() || hovered;

        auto&& style = canvas.style();
        const auto size = style.font->height();

        const bounds box = { style.padding.x, style.padding.x + size, style.padding.y, style.padding.y + size };
        canvas.push_region("box"_id, box);
        canvas.add_primitive("bounds"_id,
                             canvas_stroke_rect{ { 0.0f, size, 0.0f, size },
                                                 active ? style.highlight_color : style.normal_color,
                                                 style.bounds_edge_width });

        if(state) {
            const auto quarter = size * 0.15f;
            canvas.add_primitive("selected"_id,
                                 canvas_fill_rect{ { quarter, size - quarter, quarter, size - quarter }, style.highlight_color });
        }
        canvas.pop_region();

        primitive text =
            canvas_text{ { size + 2.0f * style.padding.x, style.padding.y }, std::move(label), style.font, style.font_color };
        const auto [w, h] = canvas.calculate_bounds(text);
        canvas.add_primitive("label"_id, std::move(text));
        canvas.pop_region(bounds{ 0.0f, w + size + 2.0f * style.padding.x, 0.0f, h + style.padding.y });
    }
    ANIMGUI_API void progressbar(canvas& canvas, const float width, const float progress, std::optional<std::pmr::string> label) {
        auto&& style = canvas.style();
        const bounds box{ 0.0f, width, 0.0f, 2.0f * style.padding.y + style.font->height() };
        canvas.push_region(mix(canvas.region_sub_uid(), "bounds"_id), box);
        canvas.add_primitive("base"_id, canvas_fill_rect{ box, style.highlight_color });
        // TODO: color mapping
        canvas.add_primitive("progress"_id,
                             canvas_fill_rect{ { 0.0f, width * progress, 0.0f, box.bottom }, style.disabled_color });
        canvas.add_primitive("bounds"_id, canvas_stroke_rect{ box, style.normal_color, style.bounds_edge_width });
        if(label.has_value()) {
            primitive text = canvas_text{ { 0.0f, style.padding.y }, std::move(label).value(), style.font, style.font_color };
            const auto text_width = canvas.calculate_bounds(text).x;
            std::get<canvas_text>(text).pos.x = (width - text_width) / 2.0f;
            canvas.add_primitive("label"_id, std::move(text));
        }

        canvas.pop_region();
    }
    ANIMGUI_API void radio_button(canvas& canvas, const std::pmr::vector<std::pmr::string>& labels, size_t& index) {
        auto&& style = canvas.style();
        canvas.push_region(canvas.region_sub_uid());
        bounds box{ 0.0f, 0.0f, 0.0f, 0.0f };
        if(index >= labels.size())
            index = 0;

        for(size_t i = 0; i < labels.size(); ++i) {
            canvas.push_region(canvas.region_sub_uid());
            const auto hovered = canvas.region_hovered() || canvas.region_request_focus();
            const auto pressed = canvas.region_pressed(key_code::left_button) || index == i;
            auto [idx, uid] = canvas.add_primitive(
                "button_base"_id,
                button_base{ { 0.0f, 0.0f }, { 0.0f, 0.0f }, hovered ? button_status::hovered : button_status::normal });
            if(clicked(canvas, uid, pressed, hovered))
                index = i;
            primitive text =
                canvas_text{ { 0.0f, 0.0f }, labels[i], style.font, index == i ? style.font_color : style.normal_color };
            const auto content_size = canvas.calculate_bounds(text);

            auto&& inst = std::get<button_base>(std::get<primitive>(canvas.commands()[idx]));
            inst.content_size = content_size;
            const auto [w, h] = canvas.calculate_bounds(inst);
            const auto offset_x = (w - content_size.x) / 2.0f;
            const auto offset_y = (h - content_size.y) / 2.0f;
            std::get<canvas_text>(text).pos = { offset_x, offset_y };

            canvas.add_primitive("label"_id, std::move(text));

            canvas.pop_region(bounds{ box.right, box.right + w, 0.0f, h });
            box.bottom = std::fmax(box.bottom, h);
            box.right += w;
        }

        canvas.pop_region(box);
    }

    template <typename T>
    static void slider_impl(canvas& canvas, const float width, const float handle_width, T& val, const T min, const T max) {
        auto&& style = canvas.style();
        const auto height = style.font->height() + 2.0f * style.padding.y;
        const auto full_uid = canvas.push_region(canvas.region_sub_uid(), bounds{ 0.0f, width, 0.0f, height }).second;

        const auto base_height = 3 * style.bounds_edge_width;

        canvas.add_primitive(
            "base"_id,
            canvas_fill_rect{ bounds{ 0.0f, width, (height - base_height) / 2.0f, (height + base_height) / 2.0f },
                              style.background_color });

        assert(min <= val && val <= max);
        auto& progress = canvas.storage<float>(full_uid);
        bool hovered = false;

        const auto half_width = handle_width / 2.0f;
        assert(width > handle_width);
        const auto scale = width - handle_width;

        if(selected(canvas, full_uid)) {
            progress = std::fmin(
                1.0f,
                std::fmax(0.0f, (canvas.input_backend().get_cursor_pos().x - canvas.region_offset().x - half_width) / scale));
            val = min + static_cast<T>(static_cast<float>(max - min) * progress);
            hovered = true;
        } else {
            progress = static_cast<float>(val - min) / static_cast<float>(max - min);
        }

        canvas.push_region("handle"_id);

        hovered |= canvas.region_hovered() | canvas.region_request_focus();

        canvas.add_primitive(
            "base"_id,
            canvas_fill_rect{ bounds{ 0.0f, handle_width, 0.0f, height }, hovered ? style.highlight_color : style.normal_color });
        const auto center = scale * progress + half_width;
        canvas.pop_region(bounds{ center - half_width, center + half_width, 0.0f, height });

        canvas.pop_region();
    }
    ANIMGUI_API void slider(canvas& canvas, const float width, const float handle_width, int32_t& val, const int32_t min,
                            const int32_t max) {
        slider_impl(canvas, width, handle_width, val, min, max);
    }
    ANIMGUI_API void slider(canvas& canvas, const float width, const float handle_width, float& val, const float min,
                            const float max) {
        slider_impl(canvas, width, handle_width, val, min, max);
    }
}  // namespace animgui
