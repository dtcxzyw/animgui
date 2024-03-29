// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/render_backend.hpp>
#include <functional>
#include <memory>

namespace animgui {
    struct image_desc;

    struct glyph_id final {
        uint32_t idx;

        explicit glyph_id(const uint32_t idx) : idx{ idx } {}
    };

    class font {
    public:
        font() = default;
        font(const font&) = delete;
        font(font&&) = default;
        font& operator=(const font&) = delete;
        font& operator=(font&&) = default;
        virtual ~font() = default;

        [[nodiscard]] virtual float height() const noexcept = 0;
        [[nodiscard]] virtual float standard_width() const noexcept = 0;
        [[nodiscard]] virtual float line_spacing() const noexcept = 0;
        [[nodiscard]] virtual glyph_id to_glyph(uint32_t codepoint) const = 0;
        [[nodiscard]] virtual float calculate_advance(glyph_id glyph, glyph_id prev) const = 0;
        [[nodiscard]] virtual bounds_aabb calculate_bounds(glyph_id glyph) const = 0;
        virtual texture_region render_to_bitmap(glyph_id glyph,
                                                const std::function<texture_region(const image_desc&)>& image_uploader) const = 0;
        [[nodiscard]] virtual float max_scale() const noexcept = 0;
    };

    // TODO: SDF
    class font_backend {
    public:
        font_backend() = default;
        virtual ~font_backend() = default;
        font_backend(const font_backend& rhs) = delete;
        font_backend(font_backend&& rhs) = default;
        font_backend& operator=(const font_backend& rhs) = delete;
        font_backend& operator=(font_backend&& rhs) = default;

        [[nodiscard]] virtual std::shared_ptr<font> load_font(const std::pmr::string& name, float height) const = 0;
    };
}  // namespace animgui
