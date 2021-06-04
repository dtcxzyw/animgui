// SPDX-License-Identifier: MIT

#pragma once
#include <cstdint>
#include <memory_resource>
#include <stack>

// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
#if _WIN32
#define ANIMGUI_WINDOWS
#elif __linux__ && !__ANDROID__
#define ANIMGUI_LINUX
#elif __APPLE__
#define ANIMGUI_MACOS
#else
#error "Unsupported platform"
#endif

#if defined(ANIMGUI_WINDOWS)
#define ANIMGUI_API
#else
#define ANIMGUI_API
#endif

namespace animgui {
    struct color final {
        float r, g, b, a;
    };
    struct vec2 final {
        float x, y;
    };
    struct uvec2 final {
        uint32_t x, y;
    };
    struct bounds final {
        float left, right, top, bottom;
    };
    inline void merge_bounds(bounds& sub, const bounds& parent) {
        sub.left += parent.left;
        sub.right += parent.right;
        sub.top += parent.top;
        sub.bottom += parent.top;
    }
    inline bool clip_bounds(bounds& sub, const bounds& parent) {
        sub.left += parent.left;
        sub.right += parent.right;
        sub.top += parent.top;
        sub.bottom += parent.top;
        sub.right = std::min(sub.right, parent.right);
        sub.bottom = std::min(sub.bottom, parent.bottom);
        return sub.left < sub.right && sub.top < sub.bottom;
    }
    struct uid final {
        uint64_t id;
        constexpr explicit uid(const uint64_t id) noexcept : id{ id } {}
        bool operator==(const uid rhs) const noexcept {
            return id == rhs.id;
        }
    };
    struct uid_hasher final {
        size_t operator()(const uid rhs) const noexcept {
            return rhs.id;
        }
    };
    inline uid mix(const uid parent, const uid child) {
        return uid{ parent.id ^ (parent.id >> 1) ^ child.id ^ (child.id * 48271) };
    }

    constexpr uid fnv1a_impl(const void* data, const size_t size) {
        // FNV-1a Hash
        auto beg = static_cast<const std::byte*>(data);
        const auto end = beg + size;
        uint64_t res = 0xcbf29ce484222325;
        while(beg < end) {
            res ^= static_cast<uint64_t>(*beg);
            res *= 0x100000001b3;
            ++beg;
        }
        return uid{ res };
    }

    // TODO: use std::span
    template <typename T>
    class span {
        T* m_begin;
        T* m_end;

    public:
        span() = delete;
        span(T* begin, T* end) noexcept : m_begin{ begin }, m_end{ end } {}
        [[nodiscard]] T* begin() const noexcept {
            return m_begin;
        }
        [[nodiscard]] T* end() const noexcept {
            return m_end;
        }
        [[nodiscard]] size_t size() const noexcept {
            return m_end - m_begin;
        }
        [[nodiscard]] span subspan(size_t offset) const noexcept {
            return { m_begin + offset, m_end };
        }
        [[nodiscard]] T& operator[](size_t idx) const noexcept {
            return m_begin[idx];
        }
    };

    template <typename T>
    using stack = std::stack<T, std::pmr::deque<T>>;
}  // namespace animgui

constexpr animgui::uid operator""_id(const char* str, const std::size_t len) {
    return animgui::fnv1a_impl(str, len);
}
