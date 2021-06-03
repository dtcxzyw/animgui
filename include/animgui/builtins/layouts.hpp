// SPDX-License-Identifier: MIT

#pragma once
#include "../core/canvas.hpp"
#include <functional>

namespace animgui {
    class layout_proxy : public canvas {
    protected:
        canvas& m_parent;
        size_t m_offset;

    public:
        explicit layout_proxy(canvas& parent) noexcept : m_parent{ parent }, m_offset{ parent.commands().size() } {}
        vec2 reserved_size() override;
        void* raw_storage(size_t hash, uid uid) final;
        [[nodiscard]] bounds region_bounds() const override;
        [[nodiscard]] bool region_hovered() const override;
        [[nodiscard]] bool region_pressed(key_code key) const override;
        void register_type(size_t hash, size_t size, size_t alignment, raw_callback ctor, raw_callback dtor) final;
        void pop_region() override;
        [[nodiscard]] bool pressed(key_code key, const bounds& bounds) const override;
        std::pair<size_t, uid> push_region(uid uid, const bounds& bounds) override;
        std::pair<size_t, uid> add_primitive(uid uid, primitive primitive) override;
        void set_cursor(cursor cursor) final;
        float step(uid id, float dest) final;
        animgui::style& style() noexcept final;
        [[nodiscard]] vec2 calculate_bounds(const primitive& primitive) const final;
        span<operation> commands() final;
        [[nodiscard]] bool hovered(const bounds& bounds) const override;
        [[nodiscard]] std::pmr::memory_resource* memory_resource() const noexcept final;
        uid region_sub_uid() override;
    };

    class row_layout_canvas : public layout_proxy {
    public:
        explicit row_layout_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void newline() = 0;
    };
    enum class row_alignment { left, right, middle, justify };

    vec2 layout_row(canvas& parent, row_alignment alignment, const std::function<void(row_layout_canvas&)>& render_function);
    bounds layout_row_center(canvas& parent, const std::function<void(row_layout_canvas&)>& render_function);

    // TODO: menu
    class multiple_window_canvas : public canvas {
    public:
        virtual void new_window(const std::function<void(canvas&)>& render_function);
    };
    void multiple_window(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);
    void docking(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);
}  // namespace animgui
