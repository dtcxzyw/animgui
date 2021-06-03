// SPDX-License-Identifier: MIT

#pragma once
#include "common.hpp"
#include <memory>
#include <unordered_map>
#include <variant>

namespace animgui {
    class font;

    class property {
        std::variant<std::monostate, std::pmr::unordered_map<uid, std::shared_ptr<property>, uid_hasher>, bool, float, int, color>
            m_value;

    public:
        property() noexcept : m_value{ std::monostate{} } {}
        template <typename T>
        explicit property(const T val) noexcept : m_value{ val } {}

        template <typename T>
        auto value_of() const {
            return std::get<T>(m_value);
        }

        [[nodiscard]] bool exists() const noexcept {
            return m_value.index() != 0;
        }

        [[nodiscard]] std::shared_ptr<property>& children(const uid uid) {
            auto&& lut = std::get<1>(m_value);
            return lut[uid];
        }
    };

    struct style {
        std::shared_ptr<font> fallback_font;
        uint32_t fallback_codepoint;
        std::shared_ptr<font> default_font;

        color fallback_background_color;
        color fallback_foreground_color;
        color fallback_font_color;

        std::shared_ptr<property> property;
    };
}  // namespace animgui
