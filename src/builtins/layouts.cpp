// SPDX-License-Identifier: MIT

#include <animgui/builtins/layouts.hpp>
#include <animgui/core/style.hpp>

namespace animgui {
    std::pmr::memory_resource* layout_proxy::memory_resource() const noexcept {
        return m_parent.memory_resource();
    }
    vec2 layout_proxy::reserved_size() {
        return m_parent.reserved_size();
    }
    bounds layout_proxy::region_bounds() const {
        return m_parent.region_bounds();
    }
    void* layout_proxy::raw_storage(const size_t hash, const uid uid) {
        return m_parent.raw_storage(hash, uid);
    }
    bool layout_proxy::region_hovered() const {
        return m_parent.region_hovered();
    }
    bool layout_proxy::region_pressed(const key_code key) const {
        return m_parent.region_pressed(key);
    }
    void layout_proxy::register_type(const size_t hash, const size_t size, const size_t alignment, const raw_callback ctor,
                                     const raw_callback dtor) {
        m_parent.register_type(hash, size, alignment, ctor, dtor);
    }
    void layout_proxy::pop_region(const std::optional<bounds>& new_bounds) {
        m_parent.pop_region(new_bounds);
    }
    std::pair<size_t, uid> layout_proxy::push_region(const uid uid, const std::optional<bounds>& reserved_bounds) {
        const auto [idx, id] = m_parent.push_region(uid, reserved_bounds);
        return { idx - m_offset, id };
    }
    bool layout_proxy::pressed(const key_code key, const bounds& bounds) const {
        return m_parent.pressed(key, bounds);
    }
    void layout_proxy::set_cursor(const cursor cursor) {
        m_parent.set_cursor(cursor);
    }
    std::pair<size_t, uid> layout_proxy::add_primitive(const uid uid, primitive primitive) {
        const auto [idx, id] = m_parent.add_primitive(uid, std::move(primitive));
        return { idx - m_offset, id };
    }

    animgui::style& layout_proxy::style() noexcept {
        return m_parent.style();
    }
    bool layout_proxy::hovered(const bounds& bounds) const {
        return m_parent.hovered(bounds);
    }
    vec2 layout_proxy::calculate_bounds(const primitive& primitive) const {
        return m_parent.calculate_bounds(primitive);
    }
    span<operation> layout_proxy::commands() {
        return m_parent.commands().subspan(m_offset);
    }
    float layout_proxy::step(const uid id, const float dest) {
        return m_parent.step(id, dest);
    }
    uid layout_proxy::region_sub_uid() {
        return m_parent.region_sub_uid();
    }

    class row_layout_canvas_impl final : public row_layout_canvas {
        row_alignment m_alignment;
        uint32_t m_current_depth = 0;
        float m_vertical_offset = 0.0f;
        float m_max_total_width = 0.0f;

        std::pmr::vector<std::tuple<uid, size_t, vec2>> m_current_line;
        void flush() {
            if(m_current_depth != 0)
                throw std::logic_error("mismatched region");

            auto max_height = 0.0f;
            auto width_sum = 0.0f;
            for(auto&& [id, idx, size] : m_current_line) {
                max_height = std::fmaxf(max_height, size.y);
                width_sum += size.x;
            }
            const auto width = reserved_size().x;
            if(!m_current_line.empty()) {
                const auto total_width = width_sum + static_cast<float>(m_current_line.size()) * style().spacing.x;
                m_max_total_width = std::fmaxf(m_max_total_width, total_width);
                auto alignment = m_alignment;
                if(alignment == row_alignment::justify &&
                   (m_current_line.size() == 1 || width < width_sum + 2.0f * style().spacing.x))
                    alignment = row_alignment::middle;
                auto offset = 0.0f;
                auto spacing = style().spacing.x;

                switch(alignment) {
                    case row_alignment::left:
                        offset = spacing;
                        break;
                    case row_alignment::right: {
                        offset = width - total_width - spacing;
                    } break;
                    case row_alignment::middle: {
                        offset = (width - total_width) / 2.0f;
                    } break;
                    case row_alignment::justify: {
                        spacing =
                            (width - width_sum - 2.0f * style().spacing.x) / (static_cast<float>(m_current_line.size()) - 1);
                        offset = style().spacing.x;
                        m_max_total_width = std::fmaxf(m_max_total_width, width);
                    } break;
                }

                const auto span = commands();
                for(auto&& [id, idx, size] : m_current_line) {
                    const auto new_bounds = bounds{ offset, offset + size.x, m_vertical_offset, m_vertical_offset + size.y };
                    std::get<animgui::push_region>(span[idx]).bounds = new_bounds;
                    storage<bounds>(mix(id, "last_bounds"_id)) = new_bounds;
                    offset += size.x + spacing;
                }
            }

            m_vertical_offset += max_height + style().spacing.y;
            m_current_line.clear();
        }

    public:
        row_layout_canvas_impl(canvas& parent, const row_alignment alignment)
            : row_layout_canvas{ parent }, m_alignment{ alignment }, m_current_line{ parent.memory_resource() } {}
        std::pair<size_t, uid> push_region(const uid uid, const std::optional<bounds>& reserved_bounds) override {
            const auto [idx, id] = layout_proxy::push_region(uid, reserved_bounds);
            if(++m_current_depth == 1) {
                m_current_line.emplace_back(id, idx, vec2{});
            }
            return { idx, id };
        }
        void pop_region(const std::optional<bounds>& new_bounds) override {
            m_parent.pop_region(new_bounds);
            if(--m_current_depth == 0) {
                const auto [x1, x2, y1, y2] = storage<bounds>(mix(std::get<uid>(m_current_line.back()), "last_bounds"_id));
                std::get<vec2>(m_current_line.back()) = { x2 - x1, y2 - y1 };
            }
        }
        void newline() override {
            flush();
        }
        vec2 finish() {
            flush();
            return { m_max_total_width, m_vertical_offset - style().spacing.y };
        }
    };

    vec2 layout_row(canvas& parent, const row_alignment alignment,
                    const std::function<void(row_layout_canvas&)>& render_function) {
        row_layout_canvas_impl canvas{ parent, alignment };
        render_function(canvas);
        return canvas.finish();
    }
}  // namespace animgui
