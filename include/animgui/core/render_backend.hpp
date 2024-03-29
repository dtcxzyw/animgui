// SPDX-License-Identifier: MIT

#pragma once
#include "common.hpp"

#include <functional>
#include <optional>
#include <variant>
#include <vector>
#include <memory>

namespace animgui {

    inline uint32_t calculate_mipmap_level(const uvec2 size) {
        return static_cast<uint32_t>(std::floor(std::log2(std::max(size.x, size.y)))) + 1;
    }

    // TODO: SDF
    enum class channel : uint32_t { alpha = 0, rgb = 1, rgba = 2 };
    struct image_desc final {
        uvec2 size;
        channel channels;
        const void* data;
    };

    class texture {
    public:
        texture() = default;
        texture(const texture&) = delete;
        texture(texture&&) = default;
        texture& operator=(const texture&) = delete;
        texture& operator=(texture&&) = default;
        virtual ~texture() = default;

        virtual void update_texture(uvec2 offset, const image_desc& image) = 0;
        virtual void generate_mipmap() = 0;
        [[nodiscard]] virtual uvec2 texture_size() const noexcept = 0;
        [[nodiscard]] virtual channel channels() const noexcept = 0;
        [[nodiscard]] virtual uint64_t native_handle() const noexcept = 0;
    };

    struct ANIMGUI_API texture_region final {
        std::shared_ptr<texture> tex;
        bounds_aabb region;
        [[nodiscard]] texture_region sub_region(const bounds_aabb& bounds) const;
    };

    using native_callback = std::function<void()>;

    enum class primitive_type : uint32_t {
        points = 1 << 0,
        lines = 1 << 1,
        line_strip = 1 << 2,
        line_loop = 1 << 3,
        triangles = 1 << 4,
        triangle_fan = 1 << 5,
        triangle_strip = 1 << 6,
        quads = 1 << 7
    };

    constexpr primitive_type operator|(const primitive_type lhs, const primitive_type rhs) {
        return static_cast<primitive_type>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    constexpr primitive_type operator&(const primitive_type lhs, const primitive_type rhs) {
        return static_cast<primitive_type>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
    }

    constexpr bool support_primitive(const primitive_type capability, const primitive_type requirement) {
        return static_cast<uint32_t>(capability) & static_cast<uint32_t>(requirement);
    }

    struct vertex final {
        vec2 pos;
        vec2 tex_coord;
        color_rgba color;
    };
    
    struct primitives final {
        primitive_type type;
        uint32_t vertices_count;
        std::shared_ptr<texture> tex;
        float point_line_size;
    };

    struct command final {
        bounds_aabb bounds;
        std::optional<bounds_aabb> clip;
        std::variant<native_callback, primitives> desc;

        command() = delete;
    };

    struct command_queue final {
        std::pmr::vector<vertex> vertices;
        std::pmr::vector<command> commands;
    };

    class render_backend {
    public:
        render_backend() = default;
        render_backend(const render_backend&) = delete;
        render_backend(render_backend&&) = default;
        render_backend& operator=(const render_backend&) = delete;
        render_backend& operator=(render_backend&&) = default;
        virtual ~render_backend() = default;

        virtual void update_command_list(uvec2 window_size, command_queue command_list) = 0;
        virtual std::shared_ptr<texture> create_texture(uvec2 size, channel channels) = 0;
        virtual std::shared_ptr<texture> create_texture_from_native_handle(uint64_t handle, uvec2 size, channel channels) = 0;
        virtual void emit(uvec2 screen_size) = 0;
        [[nodiscard]] virtual uint64_t render_time() const noexcept = 0;
        [[nodiscard]] virtual primitive_type supported_primitives() const noexcept = 0;
    };
}  // namespace animgui
