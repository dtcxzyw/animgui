// SPDX-License-Identifier: MIT

#pragma once
#include <memory>

namespace animgui {
    struct image_desc;

    class font {
    public:
        font() = default;
        font(const font&) = delete;
        font(font&&) = default;
        font& operator=(const font&) = delete;
        font& operator=(font&&) = default;
        virtual ~font() = default;

        [[nodiscard]] virtual float height() const noexcept = 0;
        [[nodiscard]] virtual float calculate_width(uint32_t codepoint) const = 0;
        [[nodiscard]] virtual bool exists(uint32_t codepoint) const = 0;
        virtual void render_to_bitmap(uint32_t codepoint, const image_desc& dest) const = 0;
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
