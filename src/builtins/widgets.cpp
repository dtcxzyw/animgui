// SPDX-License-Identifier: MIT

#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/common.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/style.hpp>
#include <cassert>
#include <cmath>
#include <utf8.h>

namespace animgui {

    text_modifier::text_modifier(canvas& parent, size_t idx)
        : m_ref(std::get<canvas_text>(std::get<primitive>(parent.commands()[idx]))) {}
    text_modifier& text_modifier::font(std::shared_ptr<animgui::font> font) {
        m_ref.font_ref = std::move(font);
        return *this;
    }
    text_modifier& text_modifier::text_color(const color_rgba& color) {
        m_ref.color = color;
        return *this;
    }

    button_label_modifier::button_label_modifier(canvas& parent, size_t idx_base, size_t idx_text, bool clicked, bool pressed, bool focused)
        : m_ref_base(std::get<canvas_fill_rect>(std::get<primitive>(parent.commands()[idx_base]))),
          m_ref_text(std::get<canvas_text>(std::get<primitive>(parent.commands()[idx_text]))),
          m_clicked{ clicked },
          m_pressed{ pressed },
          m_focused{ focused } {}
    button_label_modifier& button_label_modifier::font(std::shared_ptr<animgui::font> font) {
        m_ref_text.font_ref = std::move(font); 
        return *this;
    }
    button_label_modifier& button_label_modifier::set_style(const style& new_style) {
        m_ref_text.font_ref = new_style.default_font; 
        m_ref_text.color = new_style.primary.text;
        m_ref_base.color = m_pressed ? new_style.action.selected : (m_focused ? new_style.action.hover : new_style.primary.main);
        return *this;
    }
    button_label_modifier::operator bool() const {
        return m_clicked;
    }

    image_modifier::image_modifier(canvas& parent, size_t idx)
        : m_ref(std::get<canvas_image>(std::get<primitive>(parent.commands()[idx]))) {}
    image_modifier& image_modifier::factor(const color_rgba& factor) {
        m_ref.factor = factor;
        return *this;
    }

    button_image_modifier::button_image_modifier(canvas& parent, size_t idx_base, image_modifier img_modifier, bool clicked, bool pressed, bool focused)
        : m_ref_base(std::get<canvas_fill_rect>(std::get<primitive>(parent.commands()[idx_base]))),
          m_img_modifier{std::move(img_modifier)},
          m_clicked{ clicked },
          m_pressed{ pressed },
          m_focused{ focused } {}
    button_image_modifier& button_image_modifier::set_style(const style& new_style) {
        m_ref_base.color = m_pressed ? new_style.action.selected : (m_focused ? new_style.action.hover : new_style.primary.main);
        return *this;
    }
    button_image_modifier& button_image_modifier::factor(const color_rgba& color) {
        m_img_modifier.factor(color);
        return *this;
    }
    button_image_modifier::operator bool() const {
        return m_clicked;
    }

    slider_modifier::slider_modifier(canvas& parent, size_t idx_base, size_t idx_handle, bool focused)
        : m_ref_base(std::get<canvas_fill_rect>(std::get<primitive>(parent.commands()[idx_base]))),
          m_ref_handle(std::get<canvas_fill_rect>(std::get<primitive>(parent.commands()[idx_handle]))),
          m_focused{focused} {}
    slider_modifier& slider_modifier::set_style(const style& new_style) {
        m_ref_base.color = new_style.background;
        m_ref_handle.color = m_focused ? new_style.action.hover : new_style.primary.main;
        return *this;
    }



    ANIMGUI_API text_modifier text(canvas& parent, std::pmr::string str) {
        primitive text = canvas_text{ vec2{ 0.0f, 0.0f }, std::move(str), parent.global_style().default_font,
                                      parent.global_style().text.primary };
        const auto [w, h] = parent.calculate_bounds(text);
        parent.push_region(parent.region_sub_uid(), bounds_aabb{ 0.0f, w, 0.0f, h });
        const auto idx = parent.add_primitive("content"_id, std::move(text)).first;
        parent.pop_region();
        return text_modifier{ parent, idx };
    }
    ANIMGUI_API bool clicked(canvas& parent, const identifier id, const bool pressed, const bool focused) {
        auto& last_pressed = parent.storage<bool>(id);
        const auto res = last_pressed && !pressed && focused;
        last_pressed = pressed;
        return res;
    }
    ANIMGUI_API button_label_modifier button_label(canvas& parent, std::pmr::string label) {
        parent.push_region(parent.region_sub_uid());
        const auto focused = parent.region_request_focus() || parent.region_hovered();
        const auto pressed = focused && parent.input().action_press();
        primitive text = canvas_text{ vec2{ 0.0f, 0.0f }, std::move(label), parent.global_style().default_font, color_rgba{} };
        const auto [text_w, text_h] = parent.calculate_bounds(text);
        const auto w = text_w + parent.global_style().padding.x * 2;
        const auto h = text_h + parent.global_style().padding.y * 2;
        auto [idx_base, uid] = parent.add_primitive("button_base"_id, canvas_fill_rect{ { 0, w, 0, h }, color_rgba{} });
        const auto res = clicked(parent, uid, pressed, focused);
        parent.push_region(parent.region_sub_uid());
        const auto idx_text = parent.add_primitive("text"_id, text).first;
        const auto padding_x = parent.global_style().padding.x;
        const auto padding_y = parent.global_style().padding.y;
        parent.pop_region(bounds_aabb{ padding_x, padding_x + text_w, padding_y, padding_y + text_h });
        parent.pop_region(bounds_aabb{ 0.0f, w, 0.0f, h });
        button_label_modifier modifier{ parent, idx_base, idx_text, res, pressed, focused };
        modifier.set_style(parent.global_style());
        return modifier;
    }
    ANIMGUI_API image_modifier image(canvas& parent, texture_region image, const vec2 size, const color_rgba& factor) {
        primitive img = canvas_image{ { 0.0f, size.x, 0.0f, size.y }, std::move(image), factor };
        const auto [w, h] = parent.calculate_bounds(img);
        parent.push_region(parent.region_sub_uid(), bounds_aabb{ 0.0f, w, 0.0f, h });
        const auto idx = parent.add_primitive("content"_id, std::move(img)).first;
        parent.pop_region();
        return image_modifier{ parent, idx };
    }
    ANIMGUI_API button_image_modifier button_image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor) {
        parent.push_region(parent.region_sub_uid());
        const auto focused = parent.region_request_focus() || parent.region_hovered();
        const auto pressed = focused && parent.input().action_press();
        const auto w = size.x + parent.global_style().padding.x * 2;
        const auto h = size.y + parent.global_style().padding.y * 2;
        auto [idx_base, uid] = parent.add_primitive("button_base"_id, canvas_fill_rect{ { 0, w, 0, h }, color_rgba{} });
        const auto res = clicked(parent, uid, pressed, focused);
        parent.push_region(parent.region_sub_uid());
        image_modifier img_modifier = animgui::image(parent, std::move(image), size, factor);
        const auto padding_x = parent.global_style().padding.x;
        const auto padding_y = parent.global_style().padding.y;
        parent.pop_region(bounds_aabb{ padding_x, padding_x + size.x, padding_y, padding_y + size.y });
        parent.pop_region(bounds_aabb{ 0.0f, w, 0.0f, h });
        button_image_modifier modifier{ parent, idx_base, img_modifier, res, pressed, focused };
        modifier.set_style(parent.global_style());
        return modifier;
    }
    ANIMGUI_API bool selected(canvas& parent, const identifier id) {
        auto& [state, pressed] = parent.storage<std::pair<bool, bool>>(mix(id, "selected"_id));
        if(parent.region_hovered() && !pressed && parent.input().action_press()) {
            state = true;
        } else if(!parent.input().action_press())
            state = false;
        pressed = parent.input().action_press();
        return state;
    }
    ANIMGUI_API text_edit_status text_edit(canvas& parent, const float glyph_width, std::pmr::string& str,
                                           std::optional<std::pmr::string> placeholder) {
        auto&& style = parent.global_style();
        const auto height = style.default_font->height() + 2.0f * style.padding.y;
        const auto width = style.default_font->standard_width() * glyph_width + 2.0f * style.padding.x;
        const bounds_aabb full_bounds{ 0.0f, width, 0.0f, height };

        const auto uid = parent.push_region(parent.region_sub_uid(), full_bounds).second;

        struct edit_state final {
            bool edit;
            bool override_mode;
            size_t pos_beg;
            size_t pos_end;
            float offset;
            edit_state() noexcept : edit{ false }, override_mode{ false }, pos_beg{ 0 }, pos_end{ 0 }, offset{ 0.0f } {}
        };

        auto&& state = parent.storage<edit_state>(uid);
        auto&& [edit, override_mode, pos_beg, pos_end, offset] = state;
        bool committed = false;

        if(selected(parent, uid)) {
            // TODO: select
            edit = true;

            const auto pos = parent.input().get_cursor_pos().x;
            auto current_pos = parent.region_offset().x + style.padding.x + offset;

            pos_beg = 0;
            auto iter = str.cbegin();
            glyph_id prev{ 0 };
            while(iter != str.cend()) {
                const auto cp = utf8::next(iter, str.cend());
                const auto glyph = style.default_font->to_glyph(cp);
                const auto distance = style.default_font->calculate_advance(glyph, prev);
                current_pos += distance;
                if(current_pos > pos)
                    break;
                prev = glyph;
                ++pos_beg;
            }

            pos_end = pos_beg;

        } else if(edit && parent.input().action_press()) {
            committed = true;
            edit = false;
            offset = 0.0f;
            override_mode = false;
        }

        if(edit) {
            auto& input_backend = parent.input();
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
                input_backend.set_clipboard_text(std::pmr::string{ beg, end, parent.memory_resource() });
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

            const auto dir = input_backend.action_direction_pulse_repeated();

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
            } else if(std::fabs(-1.0f - dir.x) < 1e-3f) {
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
            } else if(std::fabs(1.0f - dir.x) < 1e-3f) {
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
                    std::pmr::string text{ parent.memory_resource() };
                    text.resize(seq.size() * 4);
                    auto iter = text.begin();
                    for(auto cp : seq)
                        iter = utf8::append(cp, iter);
                    insert_to({ text.data(), static_cast<size_t>(iter - text.begin()) }, false);
                }
            }
        }

        auto active = edit | parent.region_request_focus();  // TODO: focus

        if(parent.region_hovered()) {
            parent.input().set_cursor(cursor::edit);
            active = true;
        }

        parent.add_primitive(
            "background"_id,
            canvas_stroke_rect{ full_bounds, active ? style.action.active : style.action.disabled, style.bounds_edge_width });
        if(edit) {
            float start_pos = style.padding.x + offset;
            float end_pos = style.padding.x + offset;
            glyph_id prev{ 0 };
            size_t current_pos = 0;
            auto iter = str.cbegin();
            while(iter != str.cend()) {
                const auto cp = utf8::next(iter, str.cend());
                const auto glyph = style.default_font->to_glyph(cp);
                const auto distance = style.default_font->calculate_advance(glyph, prev);
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
                        parent.add_primitive("cursor"_id,
                                             canvas_fill_rect{ bounds_aabb{ start_pos, end_pos, style.padding.y,
                                                                            style.padding.y + style.default_font->height() },
                                                               style.action.hover });
                    } else
                        parent.add_primitive("cursor"_id,
                                             canvas_line{ { start_pos, style.padding.y },
                                                          { start_pos, style.padding.y + style.default_font->height() },
                                                          style.action.hover,
                                                          style.bounds_edge_width });
                }
            } else {
                parent.add_primitive("selected_background"_id,
                                     canvas_fill_rect{ bounds_aabb{ start_pos, end_pos, style.padding.y,
                                                                    style.padding.y + style.default_font->height() },
                                                       style.action.selected });
            }

            parent.input().set_input_candidate_window(parent.region_offset() +
                                                      vec2{ start_pos, style.padding.y + style.default_font->height() * 0.5f });
        }

        parent.push_region("text_region"_id,
                           bounds_aabb{ style.padding.x, width - style.padding.x, style.padding.y,
                                        style.padding.y + style.default_font->height() });
        auto text = !edit && str.empty() ? std::move(placeholder).value_or("") : str;
        parent.add_primitive(
            "content"_id,
            canvas_text{
                { offset, 0.0f }, std::move(text), style.default_font, str.empty() ? style.text.disabled : style.text.primary });
        parent.pop_region();

        parent.pop_region();
        return committed ? text_edit_status::committed : (edit ? text_edit_status::active : text_edit_status::inactive);
    }
    ANIMGUI_API void checkbox(canvas& parent, std::pmr::string label, bool& state) {
        const auto id = parent.push_region(parent.region_sub_uid()).second;
        const auto focused = parent.region_request_focus() || parent.region_hovered();

        if(clicked(parent, id, focused && parent.input().action_press(), focused))
            state = !state;

        auto&& style = parent.global_style();
        const auto size = style.default_font->height();

        const bounds_aabb box = { style.padding.x, style.padding.x + size, style.padding.y, style.padding.y + size };
        parent.push_region("box"_id, box);
        parent.add_primitive("bounds"_id,
                             canvas_stroke_rect{ { 0.0f, size, 0.0f, size },
                                                 focused ? style.action.hover : style.action.active,
                                                 style.bounds_edge_width });

        if(state) {
            const auto quarter = size * 0.15f;
            parent.add_primitive("selected"_id,
                                 canvas_fill_rect{ { quarter, size - quarter, quarter, size - quarter }, style.action.hover });
        }
        parent.pop_region();

        primitive text = canvas_text{
            { size + 2.0f * style.padding.x, style.padding.y }, std::move(label), style.default_font, style.text.primary
        };
        const auto [w, h] = parent.calculate_bounds(text);
        parent.add_primitive("label"_id, std::move(text));
        parent.pop_region(bounds_aabb{ 0.0f, w + size + 2.0f * style.padding.x, 0.0f, h + style.padding.y });
    }
    ANIMGUI_API void progressbar(canvas& parent, const float width, const float progress, std::optional<std::pmr::string> label) {
        auto&& style = parent.global_style();
        const bounds_aabb box{ 0.0f, width, 0.0f, 2.0f * style.padding.y + style.default_font->height() };
        parent.push_region(mix(parent.region_sub_uid(), "bounds"_id), box);
        parent.add_primitive("base"_id, canvas_fill_rect{ box, style.action.active });
        // TODO: color mapping
        parent.add_primitive("progress"_id,
                             canvas_fill_rect{ { 0.0f, width * progress, 0.0f, box.bottom }, style.action.disabled });
        parent.add_primitive("bounds"_id, canvas_stroke_rect{ box, style.action.active, style.bounds_edge_width });
        if(label.has_value()) {
            primitive text =
                canvas_text{ { 0.0f, style.padding.y }, std::move(label).value(), style.default_font, style.text.primary };
            const auto text_width = parent.calculate_bounds(text).x;
            std::get<canvas_text>(text).pos.x = (width - text_width) / 2.0f;
            parent.add_primitive("label"_id, std::move(text));
        }

        parent.pop_region();
    }
    ANIMGUI_API void radio_button(canvas& parent, const std::pmr::vector<std::pmr::string>& labels, size_t& index) {
        auto&& style = parent.global_style();
        parent.push_region(parent.region_sub_uid());
        bounds_aabb box{ 0.0f, 0.0f, 0.0f, 0.0f };
        if(index >= labels.size())
            index = 0;

        for(size_t i = 0; i < labels.size(); ++i) {
            parent.push_region(parent.region_sub_uid());
            const auto focused = parent.region_request_focus() || parent.region_hovered();
            const auto pressed = parent.input().action_press() || index == i;
            auto [idx, uid] = parent.add_primitive(
                "button_base"_id,
                button_base{ { 0.0f, 0.0f }, { 0.0f, 0.0f }, focused ? button_status::focused : button_status::normal });
            if(clicked(parent, uid, pressed, focused))
                index = i;
            primitive text = canvas_text{
                { 0.0f, 0.0f }, labels[i], style.default_font, index == i ? style.text.primary : style.text.disabled
            };
            const auto content_size = parent.calculate_bounds(text);

            auto&& inst = std::get<button_base>(std::get<primitive>(parent.commands()[idx]));
            inst.content_size = content_size;
            const auto [w, h] = parent.calculate_bounds(inst);
            const auto offset_x = (w - content_size.x) / 2.0f;
            const auto offset_y = (h - content_size.y) / 2.0f;
            std::get<canvas_text>(text).pos = { offset_x, offset_y };

            parent.add_primitive("label"_id, std::move(text));

            parent.pop_region(bounds_aabb{ box.right, box.right + w, 0.0f, h });
            box.bottom = std::fmax(box.bottom, h);
            box.right += w;
        }

        parent.pop_region(box);
    }

    template <typename T>
    static slider_modifier slider_impl(canvas& parent, const float width, const float handle_width, T& val, const T min,
                                       const T max) {
        auto&& style = parent.global_style();
        const auto height = style.default_font->height() + 2.0f * style.padding.y;
        const auto full_uid = parent.push_region(parent.region_sub_uid(), bounds_aabb{ 0.0f, width, 0.0f, height }).second;

        const auto base_height = 3 * style.bounds_edge_width;

        const size_t idx_base = parent
                                    .add_primitive("base"_id,
                                                   canvas_fill_rect{ bounds_aabb{ 0.0f, width, (height - base_height) / 2.0f,
                                                                                  (height + base_height) / 2.0f },
                                                                     style.background })
                                    .first;

        assert(min <= val && val <= max);
        auto& progress = parent.storage<float>(full_uid);
        bool focused = false;

        const auto half_width = handle_width / 2.0f;
        assert(width > handle_width);
        const auto scale = width - handle_width;

        if(selected(parent, full_uid)) {
            progress = std::fmin(
                1.0f, std::fmax(0.0f, (parent.input().get_cursor_pos().x - parent.region_offset().x - half_width) / scale));
            val = min + static_cast<T>(static_cast<float>(max - min) * progress);
            focused = true;
        } else {
            progress = static_cast<float>(val - min) / static_cast<float>(max - min);
        }

        parent.push_region("handle"_id);

        focused |= parent.region_request_focus() || parent.region_hovered();

        const size_t idx_handle =
            parent.add_primitive("base"_id, canvas_fill_rect{ bounds_aabb{ 0.0f, handle_width, 0.0f, height }, color_rgba{} })
                .first;
        const auto center = scale * progress + half_width;
        parent.pop_region(bounds_aabb{ center - half_width, center + half_width, 0.0f, height });

        parent.pop_region();

        slider_modifier modifier{ parent, idx_base, idx_handle, focused };
        modifier.set_style(style);
        return modifier;
    }
    ANIMGUI_API void slider(canvas& parent, const float width, const float min_handle_width, int32_t& val, const int32_t min,
                            const int32_t max) {
        assert(max != min);
        slider_impl(parent, width, std::fmax(width / static_cast<float>(max - min + 1), min_handle_width), val, min, max);
    }
    ANIMGUI_API void slider(canvas& parent, const float width, const float handle_width, float& val, const float min,
                            const float max) {
        assert(std::fabs(max - min) > 1e-8f);
        slider_impl(parent, width, handle_width, val, min, max);
    }
    ANIMGUI_API void switch_(canvas& parent, bool& state) {
        auto&& style = parent.global_style();
        const auto width = style.default_font->standard_width() * 3 + 2.0f * style.padding.x;
        const auto height = style.default_font->height() + 2.0f * style.padding.y;
        const auto box = bounds_aabb{ 0.0f, width * 2.0f, 0.0f, height };
        const auto uid = parent.push_region(parent.region_sub_uid(), box).second;
        const auto pressed = parent.input().action_press();
        const auto focused = parent.region_request_focus() || parent.region_hovered();
        if(clicked(parent, uid, pressed, focused)) {
            state = !state;
        }

        parent.add_primitive("base"_id, canvas_fill_rect{ box, style.background });
        const auto offset = state ? width : 0.0f;

        parent.add_primitive(
            "handle"_id,
            canvas_fill_rect{ { offset, offset + width, 0.0f, height }, focused ? style.action.hover : style.action.disabled });

        primitive text = canvas_text{ { 0.0f, style.padding.y }, state ? "ON" : "OFF", style.default_font, style.text.primary };
        const auto text_width = parent.calculate_bounds(text).x;
        std::get<canvas_text>(text).pos.x = (width - text_width) / 2.0f + offset;
        parent.add_primitive("label"_id, std::move(text));

        parent.add_primitive(
            "bounds"_id, canvas_stroke_rect{ box, focused ? style.action.hover : style.action.active, style.bounds_edge_width });

        parent.pop_region();
    }
}  // namespace animgui
