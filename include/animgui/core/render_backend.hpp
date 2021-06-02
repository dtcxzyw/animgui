// SPDX-License-Identifier: MIT

#pragma once
#include "common.hpp"

#include <functional>
#include <variant>
#include <vector>

namespace animgui {
    // TODO: SDF
    enum class channel : uint32_t { alpha, rgb, rgba };
    struct image_desc final {
        uint32_t width;
        uint32_t height;
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

        virtual void update_texture(uint32_t offset_x, uint32_t offset_y, const image_desc& image) = 0;
        [[nodiscard]] virtual std::pair<uint32_t, uint32_t> texture_size() const noexcept = 0;
        [[nodiscard]] virtual channel channels() const noexcept = 0;
        [[nodiscard]] virtual uint64_t native_handle() const noexcept = 0;
    };

    struct texture_region final {
        std::shared_ptr<texture> texture;
        bounds region;
        texture_region sub_region(const bounds& bounds) const;
    };

    struct native_drawback final {
        std::function<void(bounds)> callback;
    };

    enum class primitive_type : uint32_t { points, lines, line_strip, line_loop, triangles, triangle_fan, triangle_strip, quads };

    struct vertex final {
        vec2 pos;
        color color;
        vec2 tex_coord;
    };

    struct primitives final {
        primitive_type type;
        std::pmr::vector<vertex> vertices;
        std::shared_ptr<texture> texture;
        float point_line_size;
    };

    enum class cursor { normal, hand, horizontal, vertical, edit, cross_hair };

    struct command final {
        bounds clip;
        std::variant<native_drawback, primitives> desc;

        command() = delete;
    };

    class render_backend {
    public:
        render_backend() = default;
        render_backend(const render_backend&) = delete;
        render_backend(render_backend&&) = default;
        render_backend& operator=(const render_backend&) = delete;
        render_backend& operator=(render_backend&&) = default;
        virtual ~render_backend() = default;

        // TODO: lazy update
        virtual void update_command_list(std::pmr::vector<command> command_list) = 0;
        virtual void set_cursor(cursor cursor) = 0;
        virtual std::shared_ptr<texture> create_texture(uint32_t width, uint32_t height, channel channels) = 0;
        virtual std::shared_ptr<texture> create_texture_from_native_handle(uint64_t handle, uint32_t width, uint32_t height,
                                                                           channel channels) = 0;
        virtual void emit() = 0;
    };
}  // namespace animgui
