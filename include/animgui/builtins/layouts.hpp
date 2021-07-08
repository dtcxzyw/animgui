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
        [[nodiscard]] vec2 reserved_size() const noexcept final;
        void* raw_storage(size_t hash, identifier uid) final;
        [[nodiscard]] const bounds_aabb& region_bounds() const override;
        [[nodiscard]] bool region_hovered() const override;
        void register_type(size_t hash, size_t size, size_t alignment, raw_callback ctor, raw_callback dtor) final;
        void pop_region(const std::optional<bounds_aabb>& new_bounds) override;
        std::pair<size_t, identifier> push_region(identifier uid, const std::optional<bounds_aabb>& reserved_bounds) override;
        std::pair<size_t, identifier> add_primitive(identifier uid, primitive primitive) override;
        float step(identifier id, float dest) final;
        [[nodiscard]] const style& global_style() const noexcept final;
        [[nodiscard]] vec2 calculate_bounds(const primitive& primitive) const final;
        span<operation> commands() noexcept final;
        [[nodiscard]] bool hovered(const bounds_aabb& bounds) const override;
        [[nodiscard]] std::pmr::memory_resource* memory_resource() const noexcept final;
        identifier region_sub_uid() override;
        [[nodiscard]] input_backend& input() const noexcept override;
        bool region_request_focus(bool force) override;
        [[nodiscard]] vec2 region_offset() const final;
        [[nodiscard]] float delta_t() const noexcept final;
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
    ANIMGUI_API bounds_aabb layout_row_center(canvas& parent, const std::function<void(row_layout_canvas&)>& render_function);

    class window_canvas : public layout_proxy {
    public:
        explicit window_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void close() = 0;
        virtual void focus() = 0;
    };

    enum class window_attributes : uint32_t {
        movable = 1 << 0,
        resizable = 1 << 1,
        closable = 1 << 2,
        minimizable = 1 << 3,
        no_title_bar = 1 << 4,
        maximizable = 1 << 5
    };
    constexpr window_attributes operator|(window_attributes lhs, window_attributes rhs) {
        return static_cast<window_attributes>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }
    constexpr bool has_attribute(window_attributes attributes, window_attributes query) {
        return static_cast<uint32_t>(attributes) & static_cast<uint32_t>(query);
    }
    ANIMGUI_API void single_window(canvas& parent, std::optional<std::pmr::string> title, window_attributes attributes,
                                   const std::function<void(window_canvas&)>& render_function);

    class multiple_window_canvas : public layout_proxy {
    public:
        explicit multiple_window_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void new_window(identifier id, std::optional<std::pmr::string> title, window_attributes attributes,
                                const std::function<void(window_canvas&)>& render_function) = 0;
        virtual void close_window(identifier id) = 0;
        virtual void open_window(identifier id) = 0;
        virtual void focus_window(identifier id) = 0;
    };
    ANIMGUI_API void multiple_window(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);
    ANIMGUI_API void docking(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);

    enum class scroll_attributes : uint32_t {
        none = 0,
        vertical_scroll = 1,
        horizontal_scroll = 2,
        both = vertical_scroll | horizontal_scroll
    };

    ANIMGUI_API void panel(canvas& parent, vec2 size, scroll_attributes scroll,
                           const std::function<vec2(canvas&)>& render_function);

    class tab_canvas : public layout_proxy {
    public:
        explicit tab_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        virtual void new_tab(identifier id, std::pmr::string title, const std::function<void(canvas&)>& render_function) = 0;
        virtual void active(identifier id) = 0;
    };
    ANIMGUI_API void tab(canvas& parent, const std::function<void(tab_canvas&)>& render_function);
    ANIMGUI_API void modal(canvas& parent, vec2 size, const std::function<void(canvas&)>& render_function);
    
    ANIMGUI_API void select(canvas& parent, std::pmr::string title, bool unique,
                            const std::function<void(canvas&)>& render_function);
    ANIMGUI_API void sortable(canvas& parent, std::pmr::string title,
                              const std::function<void(row_layout_canvas&)>& render_function);
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
