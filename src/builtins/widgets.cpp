// SPDX-License-Identifier: MIT

#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/common.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/style.hpp>
#include <utf8.h>

namespace animgui {
    ANIMGUI_API void text(canvas& canvas, std::pmr::string str) {
        primitive text = canvas_text{ vec2{ 0.0f, 0.0f }, std::move(str), canvas.style().font, canvas.style().font_color };
        const auto [w, h] = canvas.calculate_bounds(text);
        canvas.push_region(canvas.region_sub_uid(), bounds{ 0.0f, w, 0.0f, h });
        canvas.add_primitive("content"_id, std::move(text));
        canvas.pop_region();
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
        auto& last_pressed = canvas.storage<bool>(uid);
        const auto clicked = last_pressed && !pressed && hovered;
        last_pressed = pressed;
        canvas.push_region(canvas.region_sub_uid());
        const auto content_size = layout_row(canvas, row_alignment::left, render_function);
        auto&& inst = std::get<button_base>(std::get<primitive>(canvas.commands()[idx]));
        inst.content_size = content_size;
        const auto [w, h] = canvas.calculate_bounds(inst);
        const auto padding_x = (w - content_size.x) / 2.0f;
        const auto padding_y = (h - content_size.y) / 2.0f;
        canvas.pop_region(bounds{ padding_x, padding_x + content_size.x, padding_y, padding_y + content_size.y });
        canvas.pop_region(bounds{ 0.0f, w, 0.0f, h });
        return clicked;
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
            bool insert;
            size_t pos_beg;
            size_t pos_end;
            float offset;
            edit_state() noexcept : edit{ false }, insert{ false }, pos_beg{ 0 }, pos_end{ 0 }, offset{ 0.0f } {}
        };

        auto&& state = canvas.storage<edit_state>(uid);
        auto&& [edit, insert, pos_beg, pos_end, offset] = state;

        if(selected(canvas, uid)) {
            edit = true;
            pos_beg = pos_end = 0;  // TODO: select
            insert = false;
        }

        if(edit) {
            auto& input_backend = canvas.input_backend();
            std::pmr::string::iterator beg, end;

            const auto update_iterator = [&] {
                beg = end = str.begin();
                utf8::advance(beg, state.pos_beg, str.end());
                utf8::advance(end, state.pos_end, str.end());
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
            const auto insert_to = [&](const std::string_view text, const bool select) {
                if(state.pos_beg != state.pos_end)
                    delete_selected();

                str.insert(beg, text.begin(), text.end());
                const auto size = utf8::distance(text.begin(), text.end());
                state.pos_end = state.pos_beg + size;
                if(!select)
                    state.pos_beg = state.pos_end;

                update_iterator();
            };

            if(input_backend.get_key_pulse(key_code::insert, false))
                insert = !insert;
            else if(input_backend.get_key_pulse(key_code::back, true)) {
                if(pos_beg == pos_end && beg != str.begin()) {
                    utf8::prior(beg, str.begin());
                    --pos_beg;
                }
                delete_selected();
            } else if(input_backend.get_key_pulse(key_code::delete_, true)) {
                if(pos_beg == pos_end && end != str.end()) {
                    utf8::next(end, str.end());
                    ++pos_end;
                }
                delete_selected();
            } else if(input_backend.get_key_pulse(key_code::left, true)) {
                if(pos_beg == pos_end) {
                    if(pos_beg >= 1) {
                        pos_end = --pos_beg;
                        utf8::prior(beg, str.begin());
                        end = beg;
                    }
                } else {
                    pos_end = pos_beg;
                    end = beg;
                }
            } else if(input_backend.get_key_pulse(key_code::right, true)) {
                if(pos_beg == pos_end) {
                    if(end != str.end()) {
                        pos_beg = ++pos_end;
                        utf8::next(end, str.end());
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
                pos_end = utf8::distance(str.begin(), str.end());
                beg = str.begin();
                end = str.end();
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
            // TODO: select
        }

        auto active = edit | canvas.region_request_focus();

        if(canvas.region_hovered()) {
            canvas.input_backend().set_cursor(cursor::edit);
            active = true;
        }

        canvas.add_primitive(
            "background"_id,
            canvas_stroke_rect{ full_bounds, active ? style.highlight_color : style.background_color, style.bounds_edge_width });
        if(edit) {
            float start_pos = style.padding.x + offset;
            float end_pos = style.padding.x + offset;
            bool is_end = true;
            glyph prev{ 0 };
            size_t current_pos = 0;
            auto iter = str.cbegin();
            while(iter != str.cend()) {
                const auto cp = utf8::next(iter, str.cend());
                const auto glyph = style.font->to_glyph(cp);
                const auto distance = style.font->calculate_advance(glyph, prev);
                if(current_pos < pos_beg)
                    start_pos += distance;
                if(current_pos < pos_end)
                    end_pos += distance;
                prev = glyph;
                ++current_pos;
                if(current_pos == pos_end) {
                    is_end = iter == str.cend();
                    break;
                }
            }

            if(pos_beg == pos_end && (!insert || is_end)) {

                if(start_pos < style.padding.x) {
                    offset += style.padding.x - start_pos;
                    start_pos = style.padding.x;
                } else if(const auto end = width - style.padding.x; start_pos > end) {
                    offset -= start_pos - end;
                    start_pos = end;
                }

                using clock = std::chrono::high_resolution_clock;
                // ReSharper disable once CppTooWideScope
                constexpr auto one_second = static_cast<uint64_t>(clock::period::den);

                if(static_cast<uint64_t>(clock::now().time_since_epoch().count()) % one_second > one_second / 2) {
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
}  // namespace animgui
