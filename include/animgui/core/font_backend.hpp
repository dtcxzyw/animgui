// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/render_backend.hpp>
#include <functional>
#include <memory>

namespace animgui {
    struct image_desc;

    struct glyph final {
        uint32_t idx;

        explicit glyph(const uint32_t idx) : idx{ idx } {}
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
        [[nodiscard]] virtual float line_spacing() const noexcept = 0;
        [[nodiscard]] virtual glyph to_glyph(uint32_t codepoint) const = 0;
        [[nodiscard]] virtual float calculate_advance(glyph glyph, animgui::glyph prev) const = 0;
        virtual bounds calculate_bounds(glyph glyph) const = 0;
        virtual texture_region render_to_bitmap(glyph glyph,
                                                const std::function<texture_region(const image_desc&)>& image_uploader) const = 0;
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
