// SPDX-License-Identifier: MIT

#pragma once
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
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
#if defined(ANIMGUI_EXPORT)
#define ANIMGUI_API __declspec(dllexport)  // NOLINT(cppcoreguidelines-macro-usage)
#else
#define ANIMGUI_API __declspec(dllimport)  // NOLINT(cppcoreguidelines-macro-usage)
#endif
#else
#define ANIMGUI_API
#endif

namespace animgui {
    struct color_rgba final {
        float r, g, b, a;
    };
    struct vec2 final {
        float x, y;
        constexpr vec2 operator+(const vec2 rhs) const noexcept {
            return { x + rhs.x, y + rhs.y };
        }
        constexpr vec2 operator-(const vec2 rhs) const noexcept {
            return { x - rhs.x, y - rhs.y };
        }
    };
    struct uvec2 final {
        uint32_t x, y;
        [[nodiscard]] constexpr bool operator!=(const uvec2 rhs) const noexcept {
            return x != rhs.x || y != rhs.y;
        }
    };
    struct bounds_aabb final {
        float left, right, top, bottom;

        [[nodiscard]] constexpr vec2 size() const noexcept {
            return { right - left, bottom - top };
        }

        static constexpr bounds_aabb escaped() noexcept {
            return { std::numeric_limits<float>::infinity(), 0.0f, 0.0f, 0.0f };
        }

        [[nodiscard]] bool is_escaped() const noexcept {
            return std::isinf(left);
        }
    };
    inline void offset_bounds(bounds_aabb& sub, const vec2 offset) {
        sub.left += offset.x;
        sub.right += offset.x;
        sub.top += offset.y;
        sub.bottom += offset.y;
    }
    inline bool clip_bounds(bounds_aabb& sub, const vec2 offset, const bounds_aabb& parent) {
        offset_bounds(sub, offset);

        sub.left = std::fmax(sub.left, parent.left);
        sub.top = std::fmax(sub.top, parent.top);
        sub.right = std::fmin(sub.right, parent.right);
        sub.bottom = std::fmin(sub.bottom, parent.bottom);
        return sub.left < sub.right && sub.top < sub.bottom;
    }

    inline bool intersect_bounds(const bounds_aabb& lhs, const bounds_aabb& rhs) {
        return std::fmax(lhs.left, rhs.left) < std::fmin(lhs.right, rhs.right) &&
            std::fmax(lhs.top, rhs.top) < std::fmin(lhs.bottom, rhs.bottom);
    }
    struct identifier final {
        uint64_t id;
        constexpr identifier() : id{ 0 } {}
        constexpr explicit identifier(const uint64_t id) noexcept : id{ id } {}
        bool operator==(const identifier rhs) const noexcept {
            return id == rhs.id;
        }
    };
    struct identifier_hasher final {
        size_t operator()(const identifier rhs) const noexcept {
            return rhs.id;
        }
    };
    inline identifier mix(const identifier parent, const identifier child) {
        return identifier{ parent.id ^ (parent.id >> 1) ^ child.id ^ (child.id * 48271) };
    }

    constexpr identifier fnv1a_impl(const void* data, const size_t size) {
        // FNV-1a Hash
        auto beg = static_cast<const std::byte*>(data);
        const auto end = beg + size;
        uint64_t res = 0xcbf29ce484222325;
        while(beg < end) {
            res ^= static_cast<uint64_t>(*beg);
            res *= 0x100000001b3;
            ++beg;
        }
        return identifier{ res };
    }

    // TODO: use std::span
    template <typename T>
    class span final {
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

    template <typename T, typename U>
    static constexpr void* offset(U T::*ptr) {
        return &(reinterpret_cast<T*>(0)->*ptr);
    }

    template <typename T, typename U>
    static constexpr uint32_t offset_u32(U T::*ptr) {
        return static_cast<uint32_t>(reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*ptr)));
    }

    using timer = std::chrono::high_resolution_clock;
    inline uint64_t current_time() {
        return timer::now().time_since_epoch().count();
    }
    constexpr uint64_t clocks_per_second() {
        return timer::period::den;
    }

}  // namespace animgui

constexpr animgui::identifier operator""_id(const char* str, const std::size_t len) {
    return animgui::fnv1a_impl(str, len);
}
