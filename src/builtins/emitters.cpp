// SPDX-License-Identifier: MIT

#include <animgui/builtins/emitters.hpp>
#include <animgui/core/common.hpp>
#include <animgui/core/emitter.hpp>
#include <animgui/core/font_backend.hpp>
#include <animgui/core/style.hpp>
#include <cmath>
#include <stack>
#include <utf8.h>

namespace animgui {
    class builtin_emitter final : public emitter {
        static vec2 calc_bounds(const button_base& item, const style& style) {
            return { item.content_size.x + 2 * style.padding.x, item.content_size.y + 2 * style.padding.y };
        }
        static color_rgba select_button_color(const button_status status, const style& style) {
            switch(status) {
                case button_status::normal:
                    return style.action.active;
                case button_status::focused:
                    return style.action.hover;
                case button_status::pressed:
                    return style.action.selected;
                case button_status::disabled:
                    return style.action.disabled;
            }
            return style.action.active;
        }
        static void emit(const button_base& item, const bounds_aabb& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, std::pmr::vector<vertex>& vertices, const style& style,
                         const std::function<texture_region(font&, glyph_id)>&) {
            auto rect = bounds_aabb{ item.anchor.x, item.anchor.x + item.content_size.x + 2 * style.padding.x, item.anchor.y,
                                     item.anchor.y + item.content_size.y + 2 * style.padding.y };
            auto render_rect = rect;
            if(!clip_bounds(rect, offset, clip_rect))
                return;
            offset_bounds(render_rect, offset);

            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto back = style.panel_background;
            const auto front = select_button_color(item.status, style);
            const auto unused = vec2{ 0.0f, 0.0f };

            commands.push_back({ render_rect, clip_rect, primitives{ primitive_type::triangle_strip, 4, nullptr, 0.0f } });
            commands.push_back(
                { render_rect, clip_rect, primitives{ primitive_type::line_loop, 4, nullptr, style.bounds_edge_width } });
            vertices.insert(vertices.end(),
                            { { p0, unused, back },
                              { p1, unused, back },
                              { p3, unused, back },
                              { p2, unused, back },
                              { p0, unused, front },
                              { p1, unused, front },
                              { p2, unused, front },
                              { p3, unused, front } });
        }
        static vec2 calc_bounds(const canvas_stroke_rect& item, const style&) {
            return { item.bounds.right - item.bounds.left + item.size, item.bounds.bottom - item.bounds.top + item.size };
        }
        static void emit(const canvas_stroke_rect& item, const bounds_aabb& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, std::pmr::vector<vertex>& vertices, const style&,
                         const std::function<texture_region(font&, glyph_id)>&) {
            if(auto rect = bounds_aabb{ item.bounds.left - item.size / 2.0f, item.bounds.right + item.size / 2.0f,
                                        item.bounds.top - item.size / 2.0f, item.bounds.bottom + item.size / 2.0f };
               !clip_bounds(rect, offset, clip_rect))
                return;
            auto render_rect = item.bounds;
            offset_bounds(render_rect, offset);
            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ render_rect, clip_rect, primitives{ primitive_type::line_loop, 4, nullptr, item.size } });
            vertices.insert(vertices.end(),
                            { { p0, unused, item.color },
                              { p1, unused, item.color },
                              { p2, unused, item.color },
                              { p3, unused, item.color } });
        }
        static vec2 calc_bounds(const canvas_fill_rect& item, const style&) {
            return { item.bounds.right - item.bounds.left, item.bounds.bottom - item.bounds.top };
        }
        static void emit(const canvas_fill_rect& item, const bounds_aabb& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, std::pmr::vector<vertex>& vertices, const style&,
                         const std::function<texture_region(font&, glyph_id)>&) {
            auto rect = item.bounds;
            if(!clip_bounds(rect, offset, clip_rect))
                return;

            auto render_rect = item.bounds;
            offset_bounds(render_rect, offset);

            const auto p0 = vec2{ rect.left, rect.top }, p1 = vec2{ rect.left, rect.bottom },
                       p2 = vec2{ rect.right, rect.bottom }, p3 = vec2{ rect.right, rect.top };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ render_rect, clip_rect, primitives{ primitive_type::triangle_strip, 4, nullptr, 0.0f } });
            vertices.insert(vertices.end(),
                            { { p0, unused, item.color },
                              { p1, unused, item.color },
                              { p3, unused, item.color },
                              { p2, unused, item.color } });
        }
        static vec2 calc_bounds(const canvas_line& item, const style&) {
            return { std::fabs(item.start.x - item.end.x) + item.size, std::fabs(item.start.y - item.end.y) + item.size };
        }
        static void emit(const canvas_line& item, const bounds_aabb& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, std::pmr::vector<vertex>& vertices, const style&,
                         const std::function<texture_region(font&, glyph_id)>&) {
            auto rect = bounds_aabb{ std::fmin(item.start.x, item.end.x) - item.size / 2.0f,
                                     std::fmax(item.start.x, item.end.x) + item.size / 2.0f,
                                     std::fmin(item.start.y, item.end.y) - item.size / 2.0f,
                                     std::fmax(item.start.y, item.end.y) + item.size / 2.0f };
            auto render_rect = rect;
            if(!clip_bounds(rect, offset, clip_rect))
                return;
            offset_bounds(render_rect, offset);

            const auto p0 = vec2{ offset.x + item.start.x, offset.y + item.start.y },
                       p1 = vec2{ offset.x + item.end.x, offset.y + item.end.y };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ render_rect, clip_rect, primitives{ primitive_type::lines, 2, nullptr, item.size } });
            vertices.insert(vertices.end(), { { p0, unused, item.color }, { p1, unused, item.color } });
        }
        static vec2 calc_bounds(const canvas_point& item, const style&) {
            return { item.size, item.size };
        }
        static void emit(const canvas_point& item, const bounds_aabb& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, std::pmr::vector<vertex>& vertices, const style&,
                         const std::function<texture_region(font&, glyph_id)>&) {
            auto rect = bounds_aabb{ item.pos.x - item.size / 2.0f, item.pos.x + item.size / 2.0f, item.pos.y - item.size / 2.0f,
                                     item.pos.y + item.size / 2.0f };
            auto render_rect = rect;
            if(!clip_bounds(rect, offset, clip_rect))
                return;
            offset_bounds(render_rect, offset);

            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ render_rect, clip_rect, primitives{ primitive_type::points, 1, nullptr, item.size } });
            vertices.push_back({ { item.pos.x + offset.x, item.pos.y + offset.y }, unused, item.color });
        }
        static vec2 calc_bounds(const canvas_image& item, const style&) {
            return { item.bounds.right - item.bounds.left, item.bounds.bottom - item.bounds.top };
        }
        static void emit(const canvas_image& item, const bounds_aabb& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, std::pmr::vector<vertex>& vertices, const style&,
                         const std::function<texture_region(font&, glyph_id)>&) {
            if(auto rect = item.bounds; !clip_bounds(rect, offset, clip_rect))
                return;
            auto render_rect = item.bounds;
            offset_bounds(render_rect, offset);

            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto [s0, s1, t0, t1] = item.tex.region;
            commands.push_back({ render_rect, clip_rect, primitives{ primitive_type::triangle_strip, 4, item.tex.tex, 0.0f } });
            vertices.insert(vertices.end(),
                            { { p0, { s0, t0 }, item.factor },
                              { p1, { s0, t1 }, item.factor },
                              { p3, { s1, t0 }, item.factor },
                              { p2, { s1, t1 }, item.factor } });
        }
        static vec2 calc_bounds(const canvas_text& item, const style&) {
            auto width = 0.0f;
            auto beg = item.str.begin();
            const auto end = item.str.end();
            glyph_id prev{ 0 };
            while(beg != end) {
                const auto cp = utf8::next(beg, end);
                const auto glyph = item.font_ref->to_glyph(cp);
                width += item.font_ref->calculate_advance(glyph, prev);
                prev = glyph;
            }
            return { width, item.font_ref->height() };
        }
        static void emit(const canvas_text& item, const bounds_aabb& clip_rect, vec2 offset, std::pmr::vector<command>& commands,
                         std::pmr::vector<vertex>& vertices, const style&,
                         const std::function<texture_region(font&, glyph_id)>& font_callback) {
            offset = offset + item.pos;
            auto beg = item.str.begin();
            const auto end = item.str.end();
            glyph_id prev{ 0 };
            while(beg != end) {
                const auto cp = utf8::next(beg, end);
                const auto glyph = item.font_ref->to_glyph(cp);
                const auto offset_x = item.font_ref->calculate_advance(glyph, prev);
                auto bounds = item.font_ref->calculate_bounds(glyph);
                prev = glyph;

                if(glyph.idx) {
                    const auto tex = font_callback(*item.font_ref, glyph);
                    auto render_bounds = bounds;
                    if(clip_bounds(bounds, offset, clip_rect)) {
                        offset_bounds(render_bounds, offset);

                        const auto p0 = vec2{ render_bounds.left, render_bounds.top },
                                   p1 = vec2{ render_bounds.left, render_bounds.bottom },
                                   p2 = vec2{ render_bounds.right, render_bounds.bottom },
                                   p3 = vec2{ render_bounds.right, render_bounds.top };
                        const auto [s0, s1, t0, t1] = tex.region;
                        commands.push_back(
                            { render_bounds, clip_rect, primitives{ primitive_type::triangle_strip, 4, tex.tex, 0.0f } });
                        vertices.insert(vertices.end(),
                                        { { p0, { s0, t0 }, item.color },
                                          { p1, { s0, t1 }, item.color },
                                          { p3, { s1, t0 }, item.color },
                                          { p2, { s1, t1 }, item.color } });
                    }
                }

                offset.x += offset_x;
                if(offset.x >= clip_rect.right)
                    break;
            }
        }
        static vec2 calc_bounds(const extended_callback& item, const style&) {
            return item.bounds;
        }
        static void emit(const extended_callback& item, const bounds_aabb& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, std::pmr::vector<vertex>& vertices, const style& style,
                         const std::function<texture_region(font&, glyph_id)>& font_callback) {
            item.emitter(clip_rect, offset, commands, style, font_callback);
        }
        std::pmr::memory_resource* m_memory_resource;

    public:
        explicit builtin_emitter(std::pmr::memory_resource* memory_resource) : m_memory_resource{ memory_resource } {}
        vec2 calculate_bounds(const primitive& primitive, const style& style) override {
            return std::visit([&style](auto&& item) { return builtin_emitter::calc_bounds(item, style); }, primitive);
        }
        command_queue transform(const vec2 size, span<operation> operations, const style& style,
                                const std::function<texture_region(font&, glyph_id)>& font_callback) override {
            std::pmr::vector<command> command_list{ m_memory_resource };
            command_list.reserve(operations.size());
            std::pmr::vector<vertex> vertices{ m_memory_resource };
            vertices.reserve(operations.size());

            std::pmr::monotonic_buffer_resource arena{ m_memory_resource };
            stack<std::pair<bounds_aabb, vec2>> clip_stack{ &arena };
            clip_stack.push({ { 0.0f, size.x, 0.0f, size.y }, { 0.0f, 0.0f } });
            uint32_t clip_discard = 0;
            stack<uint32_t> escaped_clip_discard{ &arena };
            stack<bool> escaped_stack{ &arena };

            for(auto&& operation : operations) {
                switch(operation.index()) {
                    case 0: {
                        auto cur = std::get<0>(operation).bounds;
                        escaped_stack.push(cur.is_escaped());
                        if(cur.is_escaped()) {
                            escaped_clip_discard.push(clip_discard);
                            clip_stack.push({ { 0.0f, size.x, 0.0f, size.y }, { 0.0f, 0.0f } });
                            clip_discard = 0;
                        } else {
                            const vec2 offset = { cur.left, cur.top };
                            if(clip_discard || !clip_bounds(cur, clip_stack.top().second, clip_stack.top().first))
                                ++clip_discard;
                            else {
                                clip_stack.push({ cur, clip_stack.top().second + offset });
                            }
                        }
                    } break;
                    case 1: {
                        if(escaped_stack.top()) {
                            clip_discard = escaped_clip_discard.top();
                            escaped_clip_discard.pop();
                        } else {
                            if(clip_discard)
                                --clip_discard;
                            else
                                clip_stack.pop();
                        }
                        escaped_stack.pop();
                    } break;
                    default: {
                        if(clip_discard)
                            continue;
                        const auto& clip_rect = clip_stack.top();
                        std::visit(
                            [&](auto&& item) {
                                builtin_emitter::emit(item, clip_rect.first, clip_rect.second, command_list, vertices, style,
                                                      font_callback);
                            },
                            std::get<primitive>(operation));
                    } break;
                }
            }
            return { std::move(vertices), std::move(command_list) };
        }
    };
    std::shared_ptr<emitter> create_builtin_emitter(std::pmr::memory_resource* memory_resource) {
        return std::make_shared<builtin_emitter>(memory_resource);
    }
}  // namespace animgui
