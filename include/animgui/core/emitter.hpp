// SPDX-License-Identifier: MIT

#pragma once
#include "render_backend.hpp"
#include <vector>

namespace animgui {
    struct style;
    class font;

    enum class button_status { normal, hovered, pressed, disabled };

    struct button_base final {
        vec2 anchor;
        vec2 content_size;
        button_status status;
    };

    // TODO: affine transform
    struct canvas_stroke_rect final {
        bounds bounds;
        color color;
        float size;
    };
    struct canvas_fill_rect final {
        bounds bounds;
        color color;
    };
    struct canvas_line final {
        vec2 start, end;
        color color;
        float size;
    };
    struct canvas_point final {
        vec2 pos;
        color color;
        float size;
    };
    struct canvas_image final {
        bounds bounds;
        texture_region tex;
        color factor;
    };
    struct canvas_text final {
        vec2 pos;
        std::pmr::string str;
        std::shared_ptr<font> font;
        color color;
    };

    struct extended_callback final {
        std::function<void(const bounds&, std::pmr::vector<command>&, const style&,
                           const std::function<texture_region(font&, uint32_t)>&)>
            emitter;
        vec2 bounds;
    };

    using primitive = std::variant<button_base, canvas_fill_rect, canvas_stroke_rect, canvas_line, canvas_point, canvas_image,
                                   canvas_text, extended_callback>;

    struct push_region final {
        bounds bounds;
    };
    struct pop_region final {};
    using operation = std::variant<push_region, pop_region, primitive>;

    class emitter {
    public:
        emitter() = default;
        emitter(const emitter&) = delete;
        emitter(emitter&&) = default;
        emitter& operator=(const emitter&) = delete;
        emitter& operator=(emitter&&) = default;
        virtual ~emitter() = default;

        virtual std::pmr::vector<command> transform(vec2 size, span<operation> operations, const style& style,
                                                    const std::function<texture_region(font&, uint32_t)>& font_callback) = 0;
        virtual vec2 calculate_bounds(const primitive& primitive, const style& style) = 0;
    };
}  // namespace animgui
