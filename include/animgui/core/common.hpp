// SPDX-License-Identifier: MIT

#pragma once
#include <cstdint>
#include <memory_resource>

namespace animgui {
    struct color final {
        float r, g, b, a;
    };
    struct vec2 final {
        float x, y;
    };
    struct bounds final {
        float left, right, top, bottom;
    };
    struct uid final {
        uint64_t id;
        bool operator==(const uid rhs) const noexcept {
            return id == rhs.id;
        }
    };
    struct uid_hasher final {
        size_t operator()(const uid rhs) const noexcept {
            return rhs.id;
        }
    };
    inline uid mix(const uid father, const uid children) {
        return { father.id ^ (father.id >> 1) ^ children.id ^ (children.id * 48271) };
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
        return { res };
    }
}  // namespace animgui

constexpr animgui::uid operator""_id(const char* str) {
    auto end = str;
    while(*end)
        ++end;
    return animgui::fnv1a_impl(str, end - str);
}
