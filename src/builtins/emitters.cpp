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
                case button_status::hovered:
                    return style.highlight_color;
                default:
                    return style.normal_color;
            }
        }
        static void emit(const button_base& item, const bounds& clip_rect, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, uint32_t)>& font_callback) {
            auto rect = bounds{ item.anchor.x, item.anchor.y, item.anchor.x + item.content_size.x + 2 * style.padding.x,
                                item.anchor.y + item.content_size.y + 2 * style.padding.y };
            auto render_rect = rect;
            if(!clip_bounds(rect, clip_rect))
                return;
            merge_bounds(render_rect, clip_rect);
            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto back = style.background_color;
            const auto front = select_button_color(item.status, style);
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back(
                { clip_rect,
                  primitives{ primitive_type::triangle_strip,
                              { { { p0, back, unused }, { p1, back, unused }, { p3, back, unused }, { p2, back, unused } },
                                commands.get_allocator().resource() },
                              nullptr,
                              0.0f } });
            commands.push_back(
                { clip_rect,
                  primitives{ primitive_type::line_loop,
                              { { { p0, front, unused }, { p1, front, unused }, { p2, front, unused }, { p3, front, unused } },
                                commands.get_allocator().resource() },
                              nullptr,
                              1.0f } });
        }
        static vec2 calc_bounds(const canvas_stroke_rect& item, const style& style) {
            return { item.bounds.right - item.bounds.left + item.size, item.bounds.bottom - item.bounds.top + item.size };
        }
        static void emit(const canvas_stroke_rect& item, const bounds& clip_rect, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, uint32_t)>& font_callback) {
            if(auto rect = bounds{ item.bounds.left - item.size / 2.0f, item.bounds.right + item.size / 2.0f,
                                   item.bounds.top - item.size / 2.0f, item.bounds.bottom + item.size / 2.0f };
               !clip_bounds(rect, clip_rect))
                return;
            auto render_rect = item.bounds;
            merge_bounds(render_rect, clip_rect);
            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ clip_rect,
                                 primitives{ primitive_type::line_loop,
                                             { { { p0, item.color, unused },
                                                 { p1, item.color, unused },
                                                 { p2, item.color, unused },
                                                 { p3, item.color, unused } },
                                               commands.get_allocator().resource() },
                                             nullptr,
                                             item.size } });
        }
        static vec2 calc_bounds(const canvas_fill_rect& item, const style& style) {
            return { item.bounds.right - item.bounds.left, item.bounds.bottom - item.bounds.top };
        }
        static void emit(const canvas_fill_rect& item, const bounds& clip_rect, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, uint32_t)>& font_callback) {
            auto rect = item.bounds;
            if(!clip_bounds(rect, clip_rect))
                return;
            const auto p0 = vec2{ rect.left, rect.top }, p1 = vec2{ rect.left, rect.bottom },
                       p2 = vec2{ rect.right, rect.bottom }, p3 = vec2{ rect.right, rect.top };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back({ clip_rect,
                                 primitives{ primitive_type::triangle_strip,
                                             { { { p0, item.color, unused },
                                                 { p1, item.color, unused },
                                                 { p3, item.color, unused },
                                                 { p2, item.color, unused } },
                                               commands.get_allocator().resource() },
                                             nullptr,
                                             0.0f } });
        }
        static vec2 calc_bounds(const canvas_line& item, const style& style) {
            return { std::fabsf(item.start.x - item.end.x) + item.size, std::fabsf(item.start.y - item.end.y) + item.size };
        }
        static void emit(const canvas_line& item, const bounds& clip_rect, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, uint32_t)>& font_callback) {
            if(auto rect = bounds{ std::fminf(item.start.x, item.end.x) - item.size / 2.0f,
                                   std::fmaxf(item.start.x, item.end.x) + item.size / 2.0f,
                                   std::fminf(item.start.y, item.end.y) - item.size / 2.0f,
                                   std::fmaxf(item.start.y, item.end.y) + item.size / 2.0f };
               !clip_bounds(rect, clip_rect))
                return;
            const auto p0 = vec2{ clip_rect.left + item.start.x, clip_rect.top + item.start.y },
                       p1 = vec2{ clip_rect.left + item.end.x, clip_rect.top + item.end.y };
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back(
                { clip_rect,
                  primitives{ primitive_type::lines,
                              { { { p0, item.color, unused }, { p1, item.color, unused } }, commands.get_allocator().resource() },
                              nullptr,
                              item.size } });
        }
        static vec2 calc_bounds(const canvas_point& item, const style& style) {
            return { item.size, item.size };
        }
        static void emit(const canvas_point& item, const bounds& clip_rect, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, uint32_t)>& font_callback) {
            if(auto rect = bounds{ item.pos.x - item.size / 2.0f, item.pos.x + item.size / 2.0f, item.pos.y - item.size / 2.0f,
                                   item.pos.y + item.size / 2.0f };
               !clip_bounds(rect, clip_rect))
                return;
            const auto unused = vec2{ 0.0f, 0.0f };
            commands.push_back(
                { clip_rect,
                  primitives{ primitive_type::points,
                              { { { { item.pos.x + clip_rect.left, item.pos.y + clip_rect.top }, item.color, unused } },
                                commands.get_allocator().resource() },
                              nullptr,
                              item.size } });
        }
        static vec2 calc_bounds(const canvas_image& item, const style& style) {
            return { item.bounds.right - item.bounds.left, item.bounds.bottom - item.bounds.top };
        }
        static void emit(const canvas_image& item, const bounds& clip_rect, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, uint32_t)>& font_callback) {
            if(auto rect = item.bounds; !clip_bounds(rect, clip_rect))
                return;
            auto render_rect = item.bounds;
            merge_bounds(render_rect, clip_rect);

            const auto p0 = vec2{ render_rect.left, render_rect.top }, p1 = vec2{ render_rect.left, render_rect.bottom },
                       p2 = vec2{ render_rect.right, render_rect.bottom }, p3 = vec2{ render_rect.right, render_rect.top };
            const auto [s0, s1, t0, t1] = item.tex.region;
            commands.push_back({ clip_rect,
                                 primitives{ primitive_type::triangle_strip,
                                             { { { p0, item.factor, { s0, t0 } },
                                                 { p1, item.factor, { s0, t1 } },
                                                 { p3, item.factor, { s1, t0 } },
                                                 { p2, item.factor, { s1, t1 } } },
                                               commands.get_allocator().resource() },
                                             item.tex.texture,
                                             0.0f } });
        }
        static vec2 calc_bounds(const canvas_text& item, const style& style) {
            float width = 0.0f;
            auto beg = item.str.begin();
            const auto end = item.str.end();
            // TODO: handle fallback characters
            while(const auto cp = utf8::next(beg, end))
                width += item.font->calculate_width(cp);

            return { width, item.font->height() };
        }
        static void emit(const canvas_text& item, const bounds& clip_rect, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, uint32_t)>& font_callback) {
            auto rect = clip_rect;
            auto beg = item.str.begin();
            const auto end = item.str.end();
            while(const auto cp = utf8::next(beg, end)) {
                const auto tex = item.font->exists(cp) ?
                    font_callback(*item.font, cp) :
                    font_callback(*style.fallback_font, style.fallback_font->exists(cp) ? cp : style.fallback_codepoint);
                const auto w = item.font->calculate_width(cp);
                const auto h = item.font->height();
                const auto [left, right, top, bottom] = bounds{ rect.left, rect.left + w, rect.top, rect.top + h };
                if(left >= clip_rect.left)
                    break;

                const auto p0 = vec2{ left, top }, p1 = vec2{ left, bottom }, p2 = vec2{ right, bottom }, p3 = vec2{ right, top };
                const auto [s0, s1, t0, t1] = tex.region;
                commands.push_back({ clip_rect,
                                     primitives{ primitive_type::triangle_strip,
                                                 { { { p0, item.color, { s0, t0 } },
                                                     { p1, item.color, { s0, t1 } },
                                                     { p3, item.color, { s1, t0 } },
                                                     { p2, item.color, { s1, t1 } } },
                                                   commands.get_allocator().resource() },
                                                 tex.texture,
                                                 0.0f } });
                rect.left += w;
            }
        }
        static vec2 calc_bounds(const extended_callback& item, const style& style) {
            return item.bounds;
        }
        static void emit(const extended_callback& item, const bounds& clip_rect, std::pmr::vector<command>& commands,
                         const style& style, const std::function<texture_region(font&, uint32_t)>& font_callback) {
            item.emitter(clip_rect, commands, style, font_callback);
        }
        std::pmr::memory_resource* m_memory_resource;

    public:
        explicit builtin_emitter(std::pmr::memory_resource* memory_resource) : m_memory_resource{ memory_resource } {}
        vec2 calculate_bounds(const primitive& primitive, const style& style) override {
            return std::visit([&style](auto&& item) { return builtin_emitter::calc_bounds(item, style); }, primitive);
        }
        std::pmr::vector<command> transform(const vec2 size, span<operation> operations, const style& style,
                                            const std::function<texture_region(font&, uint32_t)>& font_callback) override {
            std::pmr::vector<command> command_list{ m_memory_resource };
            stack<bounds> clip_stack{ m_memory_resource };
            clip_stack.push({ 0.0f, size.x, 0.0f, size.y });
            uint32_t clip_discard = 0;

            for(auto&& operation : operations) {
                switch(operation.index()) {
                    case 0: {
                        if(auto cur = std::get<0>(operation).bounds; clip_discard || !clip_bounds(cur, clip_stack.top()))
                            ++clip_discard;
                        else
                            clip_stack.push(cur);
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
                            [&](auto&& item) { builtin_emitter::emit(item, clip_rect, command_list, style, font_callback); },
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
