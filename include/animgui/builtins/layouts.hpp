// SPDX-License-Identifier: MIT

#pragma once
#include "../core/canvas.hpp"
#include <functional>
#include <optional>

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
        void pop_region(const std::optional<bounds>& new_bounds) override;
        [[nodiscard]] bool pressed(key_code key, const bounds& bounds) const override;
        std::pair<size_t, uid> push_region(uid uid, const std::optional<bounds>& reserved_bounds) override;
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

    // TODO: indent/separate
    class row_layout_canvas : public layout_proxy {
    public:
        explicit row_layout_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void newline() = 0;
    };
    enum class row_alignment { left, right, middle, justify };

    ANIMGUI_API vec2 layout_row(canvas& parent, row_alignment alignment,
                                const std::function<void(row_layout_canvas&)>& render_function);
    ANIMGUI_API bounds layout_row_center(canvas& parent, const std::function<void(row_layout_canvas&)>& render_function);

    class window_canvas : public layout_proxy {
    public:
        explicit window_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void new_menu_item(const std::function<void(canvas&)>& render_function) = 0;
    };

    enum class window_attributes : uint32_t { movable = 1 << 0, resizable = 1 << 1, closable = 1 << 2, minimizable = 1 << 3 };

    class multiple_window_canvas : public layout_proxy {
    public:
        explicit multiple_window_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void new_window(uid id, std::optional<std::pmr::string> title, window_attributes attributes,
                                const std::function<void(window_canvas&)>& render_function) = 0;
    };
    ANIMGUI_API void multiple_window(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);
    ANIMGUI_API void docking(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);

    void panel(canvas& parent, vec2 size, const std::function<void(canvas&)>& render_function);

    class tab_canvas : public layout_proxy {
    public:
        explicit tab_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void new_tab(uid id, std::pmr::string title, const std::function<void(canvas&)>& render_function) = 0;
        virtual void active(uid id) = 0;
    };
    ANIMGUI_API void tab(canvas& parent, const std::function<void(tab_canvas&)>& render_function);
    ANIMGUI_API void modal(canvas& parent, vec2 size, const std::function<void(canvas&)>& render_function);

    ANIMGUI_API void layout_vertical(canvas& parent, float max_height, const std::function<void(canvas&)>& render_function);
    ANIMGUI_API void select(canvas& parent, std::pmr::string title, bool unique,
                            const std::function<void(canvas&)>& render_function);
    ANIMGUI_API void group(canvas& parent, const std::function<void(canvas&)>& render_function);
    ANIMGUI_API void popup(canvas& parent, std::pmr::string title, const std::function<void(canvas&)>& render_function);
    class table_canvas : public layout_proxy {
    public:
        explicit table_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void next_item() = 0;
    };
    // TODO: complex header
    ANIMGUI_API void table(canvas& parent, const std::pmr::vector<row_alignment>& columns,
                           const std::function<void(table_canvas&)>& render_function);
    ANIMGUI_API void tree(canvas& parent, const std::function<void(canvas&)>& render_function);

}  // namespace animgui
