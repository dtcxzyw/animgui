// SPDX-License-Identifier: MIT

#pragma once
#include "render_backend.hpp"

#include <vector>

namespace animgui {
    class font;

    struct button_base final {
        vec2 content_size;
        color color;
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
    };
    struct canvas_point final {
        vec2 pos;
        color color;
    };
    struct canvas_image final {
        texture_region tex;
        color factor;
    };
    struct canvas_text final {
        std::pmr::string str;
        std::shared_ptr<font> font;
        color color;
    };

    struct extended_callback final {
        std::pmr::vector<command> command_list;
        bounds bounds;
    };

    // TODO: shadow/motion blur?
    struct primitive final {
        vec2 base;
        std::variant<button_base, canvas_fill_rect, canvas_stroke_rect, canvas_line, canvas_point, canvas_image, canvas_text>
            desc;

        primitive() = delete;
    };

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

        virtual std::pmr::vector<command> transform(std::pmr::vector<operation> operations) = 0;
        virtual bounds calculate_bounds(primitive primitive) = 0;
    };
}  // namespace animgui
