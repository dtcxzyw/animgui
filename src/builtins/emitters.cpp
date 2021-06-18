// SPDX-License-Identifier: MIT

#include <animgui/builtins/emitters.hpp>
#include <animgui/core/common.hpp>
#include <animgui/core/emitter.hpp>
#include <animgui/core/font_backend.hpp>
#include <animgui/core/style.hpp>
#include <stack>
#include <utf8.h>

namespace animgui {
    class builtin_emitter final : public emitter {
        static vec2 calc_bounds(const button_base& item, const style& style) {
            return { item.content_size.x + 2 * style.padding.x, item.content_size.y + 2 * style.padding.y };
        }
        static color select_button_color(const button_status status, const style& style) {
            switch(status) {
                case button_status::disabled:
                    return style.disabled_color;
                case button_status::focused:
                    return style.highlight_color;
                default:
                    return style.normal_color;
            }
        }
        static void emit(const button_base& item, const bounds& clip_rect, const vec2 offset, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, glyph)>&) {
            auto rect = bounds{ item.anchor.x, item.anchor.x + item.content_size.x + 2 * style.padding.x, item.anchor.y,
                                item.anchor.y + item.content_size.y + 2 * style.padding.y };
            auto render_rect = rect;
            if(!clip_bounds(rect, offset, clip_rect))
                return;
            offset_bounds(render_rect, offset);

            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto back = style.background_color;
            const auto front = select_button_color(item.status, style);
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back(
                { clip_rect,
                  primitives{ primitive_type::triangle_strip,
                              { { { p0, unused, back }, { p1, unused, back }, { p3, unused, back }, { p2, unused, back } },
                                commands.get_allocator().resource() },
                              nullptr,
                              0.0f } });
            commands.push_back(
                { clip_rect,
                  primitives{ primitive_type::line_loop,
                              { { { p0, unused, front }, { p1, unused, front }, { p2, unused, front }, { p3, unused, front } },
                                commands.get_allocator().resource() },
                              nullptr,
                              style.bounds_edge_width } });
        }
        static vec2 calc_bounds(const canvas_stroke_rect& item, const style&) {
            return { item.bounds.right - item.bounds.left + item.size, item.bounds.bottom - item.bounds.top + item.size };
        }
        static void emit(const canvas_stroke_rect& item, const bounds& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, const style&, const std::function<texture_region(font&, glyph)>&) {
            if(auto rect = bounds{ item.bounds.left - item.size / 2.0f, item.bounds.right + item.size / 2.0f,
                                   item.bounds.top - item.size / 2.0f, item.bounds.bottom + item.size / 2.0f };
               !clip_bounds(rect, offset, clip_rect))
                return;
            auto render_rect = item.bounds;
            offset_bounds(render_rect, offset);
            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ clip_rect,
                                 primitives{ primitive_type::line_loop,
                                             { { { p0, unused, item.color },
                                                 { p1, unused, item.color },
                                                 { p2, unused, item.color },
                                                 { p3, unused, item.color } },
                                               commands.get_allocator().resource() },
                                             nullptr,
                                             item.size } });
        }
        static vec2 calc_bounds(const canvas_fill_rect& item, const style&) {
            return { item.bounds.right - item.bounds.left, item.bounds.bottom - item.bounds.top };
        }
        static void emit(const canvas_fill_rect& item, const bounds& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, const style&, const std::function<texture_region(font&, glyph)>&) {
            auto rect = item.bounds;
            if(!clip_bounds(rect, offset, clip_rect))
                return;
            const auto p0 = vec2{ rect.left, rect.top }, p1 = vec2{ rect.left, rect.bottom },
                       p2 = vec2{ rect.right, rect.bottom }, p3 = vec2{ rect.right, rect.top };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ clip_rect,
                                 primitives{ primitive_type::triangle_strip,
                                             { { { p0, unused, item.color },
                                                 { p1, unused, item.color },
                                                 { p3, unused, item.color },
                                                 { p2, unused, item.color } },
                                               commands.get_allocator().resource() },
                                             nullptr,
                                             0.0f } });
        }
        static vec2 calc_bounds(const canvas_line& item, const style&) {
            return { std::fabsf(item.start.x - item.end.x) + item.size, std::fabsf(item.start.y - item.end.y) + item.size };
        }
        static void emit(const canvas_line& item, const bounds& clip_rect, const vec2 offset, std::pmr::vector<command>& commands,
                         const style&, const std::function<texture_region(font&, glyph)>&) {
            if(auto rect = bounds{ std::fminf(item.start.x, item.end.x) - item.size / 2.0f,
                                   std::fmaxf(item.start.x, item.end.x) + item.size / 2.0f,
                                   std::fminf(item.start.y, item.end.y) - item.size / 2.0f,
                                   std::fmaxf(item.start.y, item.end.y) + item.size / 2.0f };
               !clip_bounds(rect, offset, clip_rect))
                return;
            const auto p0 = vec2{ offset.x + item.start.x, offset.y + item.start.y },
                       p1 = vec2{ offset.x + item.end.x, offset.y + item.end.y };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back(
                { clip_rect,
                  primitives{ primitive_type::lines,
                              { { { p0, unused, item.color }, { p1, unused, item.color } }, commands.get_allocator().resource() },
                              nullptr,
                              item.size } });
        }
        static vec2 calc_bounds(const canvas_point& item, const style&) {
            return { item.size, item.size };
        }
        static void emit(const canvas_point& item, const bounds& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, const style&, const std::function<texture_region(font&, glyph)>&) {
            if(auto rect = bounds{ item.pos.x - item.size / 2.0f, item.pos.x + item.size / 2.0f, item.pos.y - item.size / 2.0f,
                                   item.pos.y + item.size / 2.0f };
               !clip_bounds(rect, offset, clip_rect))
                return;
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ clip_rect,
                                 primitives{ primitive_type::points,
                                             { { { { item.pos.x + offset.x, item.pos.y + offset.y }, unused, item.color } },
                                               commands.get_allocator().resource() },
                                             nullptr,
                                             item.size } });
        }
        static vec2 calc_bounds(const canvas_image& item, const style&) {
            return { item.bounds.right - item.bounds.left, item.bounds.bottom - item.bounds.top };
        }
        static void emit(const canvas_image& item, const bounds& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, const style&, const std::function<texture_region(font&, glyph)>&) {
            if(auto rect = item.bounds; !clip_bounds(rect, offset, clip_rect))
                return;
            auto render_rect = item.bounds;
            offset_bounds(render_rect, offset);

            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto [s0, s1, t0, t1] = item.tex.region;
            commands.push_back({ clip_rect,
                                 primitives{ primitive_type::triangle_strip,
                                             { { { p0, { s0, t0 }, item.factor },
                                                 { p1, { s0, t1 }, item.factor },
                                                 { p3, { s1, t0 }, item.factor },
                                                 { p2, { s1, t1 }, item.factor } },
                                               commands.get_allocator().resource() },
                                             item.tex.texture,
                                             0.0f } });
        }
        static vec2 calc_bounds(const canvas_text& item, const style&) {
            auto width = 0.0f;
            auto beg = item.str.begin();
            const auto end = item.str.end();
            glyph prev{ 0 };
            while(beg != end) {
                const auto cp = utf8::next(beg, end);
                const auto glyph = item.font->to_glyph(cp);
                width += item.font->calculate_advance(glyph, prev);
                prev = glyph;
            }
            return { width, item.font->height() };
        }
        static void emit(const canvas_text& item, const bounds& clip_rect, vec2 offset, std::pmr::vector<command>& commands,
                         const style&, const std::function<texture_region(font&, glyph)>& font_callback) {
            offset = offset + item.pos;
            auto beg = item.str.begin();
            const auto end = item.str.end();
            glyph prev{ 0 };
            while(beg != end) {
                const auto cp = utf8::next(beg, end);
                const auto glyph = item.font->to_glyph(cp);
                const auto offset_x = item.font->calculate_advance(glyph, prev);
                auto bounds = item.font->calculate_bounds(glyph);
                prev = glyph;

                if(glyph.idx) {
                    const auto tex = font_callback(*item.font, glyph);
                    auto render_bounds = bounds;
                    if(clip_bounds(bounds, offset, clip_rect)) {
                        offset_bounds(render_bounds, offset);

                        const auto p0 = vec2{ render_bounds.left, render_bounds.top },
                                   p1 = vec2{ render_bounds.left, render_bounds.bottom },
                                   p2 = vec2{ render_bounds.right, render_bounds.bottom },
                                   p3 = vec2{ render_bounds.right, render_bounds.top };
                        const auto [s0, s1, t0, t1] = tex.region;
                        commands.push_back({ clip_rect,
                                             primitives{ primitive_type::triangle_strip,
                                                         { { { p0, { s0, t0 }, item.color },
                                                             { p1, { s0, t1 }, item.color },
                                                             { p3, { s1, t0 }, item.color },
                                                             { p2, { s1, t1 }, item.color } },
                                                           commands.get_allocator().resource() },
                                                         tex.texture,
                                                         0.0f } });
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
        static void emit(const extended_callback& item, const bounds& clip_rect, const vec2 offset,
                         std::pmr::vector<command>& commands, const style& style,
                         const std::function<texture_region(font&, glyph)>& font_callback) {
            item.emitter(clip_rect, offset, commands, style, font_callback);
        }
        std::pmr::memory_resource* m_memory_resource;

    public:
        explicit builtin_emitter(std::pmr::memory_resource* memory_resource) : m_memory_resource{ memory_resource } {}
        vec2 calculate_bounds(const primitive& primitive, const style& style) override {
            return std::visit([&style](auto&& item) { return builtin_emitter::calc_bounds(item, style); }, primitive);
        }
        std::pmr::vector<command> transform(const vec2 size, span<operation> operations, const style& style,
                                            const std::function<texture_region(font&, glyph)>& font_callback) override {
            std::pmr::vector<command> command_list{ m_memory_resource };
            std::pmr::monotonic_buffer_resource arena{ m_memory_resource };
            stack<std::pair<bounds, vec2>> clip_stack{ &arena };
            clip_stack.push({ { 0.0f, size.x, 0.0f, size.y }, { 0.0f, 0.0f } });
            uint32_t clip_discard = 0;

            for(auto&& operation : operations) {
                switch(operation.index()) {
                    case 0: {
                        auto cur = std::get<0>(operation).bounds;
                        const vec2 offset = { cur.left, cur.top };
                        if(clip_discard || !clip_bounds(cur, clip_stack.top().second, clip_stack.top().first))
                            ++clip_discard;
                        else {
                            clip_stack.push({ cur, clip_stack.top().second + offset });
                        }
                    } break;
                    case 1: {
                        if(clip_discard)
                            --clip_discard;
                        else
                            clip_stack.pop();
                    } break;
                    default: {
                        if(clip_discard)
                            continue;
                        const auto& clip_rect = clip_stack.top();
                        std::visit(
                            [&](auto&& item) {
                                builtin_emitter::emit(item, clip_rect.first, clip_rect.second, command_list, style,
                                                      font_callback);
                            },
                            std::get<primitive>(operation));
                    } break;
                }
            }
            return command_list;
        }
    };
    std::shared_ptr<emitter> create_builtin_emitter(std::pmr::memory_resource* memory_resource) {
        return std::make_shared<builtin_emitter>(memory_resource);
    }
}  // namespace animgui
