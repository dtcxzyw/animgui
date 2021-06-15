// SPDX-License-Identifier: MIT

#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/style.hpp>

namespace animgui {
    std::pmr::memory_resource* layout_proxy::memory_resource() const noexcept {
        return m_parent.memory_resource();
    }
    vec2 layout_proxy::reserved_size() const noexcept {
        return m_parent.reserved_size();
    }
    const bounds& layout_proxy::region_bounds() const {
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
    std::pair<size_t, uid> layout_proxy::add_primitive(const uid uid, primitive primitive) {
        const auto [idx, id] = m_parent.add_primitive(uid, std::move(primitive));
        return { idx - m_offset, id };
    }
    bool layout_proxy::region_request_focus(const bool force) {
        return m_parent.region_request_focus(force);
    }

    const style& layout_proxy::style() const noexcept {
        return m_parent.style();
    }
    bool layout_proxy::hovered(const bounds& bounds) const {
        return m_parent.hovered(bounds);
    }
    vec2 layout_proxy::calculate_bounds(const primitive& primitive) const {
        return m_parent.calculate_bounds(primitive);
    }
    span<operation> layout_proxy::commands() noexcept {
        return m_parent.commands().subspan(m_offset);
    }
    float layout_proxy::step(const uid id, const float dest) {
        return m_parent.step(id, dest);
    }
    uid layout_proxy::region_sub_uid() {
        return m_parent.region_sub_uid();
    }
    input_backend& layout_proxy::input_backend() const noexcept {
        return m_parent.input_backend();
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
                const auto total_width =
                    width_sum + std::fmax(0.0f, static_cast<float>(m_current_line.size()) - 1.0f) * style().spacing.x;
                m_max_total_width = std::fmaxf(m_max_total_width, total_width);
                auto alignment = m_alignment;
                if(alignment == row_alignment::justify && (m_current_line.size() == 1 || width < width_sum))
                    alignment = row_alignment::middle;
                auto offset = 0.0f;
                auto spacing = style().spacing.x;

                switch(alignment) {
                    case row_alignment::left:
                        break;
                    case row_alignment::right: {
                        offset = width - total_width;
                    } break;
                    case row_alignment::middle: {
                        offset = (width - total_width) / 2.0f;
                    } break;
                    case row_alignment::justify: {
                        spacing = (width - width_sum) / (static_cast<float>(m_current_line.size()) - 1);
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

    ANIMGUI_API vec2 layout_row(canvas& parent, const row_alignment alignment,
                                const std::function<void(row_layout_canvas&)>& render_function) {
        row_layout_canvas_impl canvas{ parent, alignment };
        render_function(canvas);
        return canvas.finish();
    }

    ANIMGUI_API bounds layout_row_center(canvas& parent, const std::function<void(row_layout_canvas&)>& render_function) {
        parent.push_region("layout_center_region"_id);
        const auto [w, h] = layout_row(parent, row_alignment::middle, render_function);
        const auto offset_y = (parent.reserved_size().y - h) / 2.0f;
        const auto bounds = animgui::bounds{ 0.0f, parent.reserved_size().x, offset_y, offset_y + h };
        parent.pop_region(bounds);
        return bounds;
    }

    // TODO: scroll
    ANIMGUI_API void panel(canvas& parent, const vec2 size, const std::function<void(canvas&)>& render_function) {
        const bounds bounds{ 0.0f, size.x, 0.0f, size.y };
        parent.push_region(parent.region_sub_uid(), bounds);
        parent.add_primitive(parent.region_sub_uid(), canvas_stroke_rect{ bounds, parent.style().highlight_color, 5.0f });
        render_function(parent);
        parent.pop_region();
    }

    class window_operator {
    public:
        window_operator() = default;
        virtual ~window_operator() = default;
        window_operator(const window_operator& rhs) = delete;
        window_operator(window_operator&& rhs) = default;
        window_operator& operator=(const window_operator& rhs) = delete;
        window_operator& operator=(window_operator&& rhs) = default;

        virtual void close() = 0;
        virtual void minimize() = 0;
        virtual void maximize() = 0;
        virtual void move(vec2 delta) = 0;
        virtual void focus() = 0;
    };

    class window_canvas_impl final : public window_canvas {
        window_attributes m_attributes;
        window_operator& m_operator;
        vec2 m_size;

    public:
        window_canvas_impl(canvas& parent, const bounds& bounds, std::optional<std::pmr::string> title,
                           const window_attributes attributes, window_operator& operator_)
            : window_canvas{ parent }, m_attributes{ attributes }, m_operator{ operator_ }, m_size{ bounds.size() } {
            layout_proxy::push_region("window"_id, bounds);

            layout_proxy::add_primitive("window_background"_id,
                                        canvas_fill_rect{ { 0.0f, m_size.x, 0.0f, m_size.y }, style().panel_background_color });
            layout_proxy::add_primitive("window_bounds"_id,
                                        canvas_stroke_rect{ { 0.0f, m_size.x, 0.0f, m_size.y }, style().highlight_color, 5.0f });
            const auto height = style().font->height() * 1.1f;
            if(!has_attribute(m_attributes, window_attributes::no_title_bar)) {
                const animgui::bounds bar{ 0.0f, m_size.x, 0.0f, height };
                const auto bar_uid = layout_proxy::push_region("title_bar"_id, bar).second;

                bool& move = storage<bool>(bar_uid);
                if(layout_proxy::region_pressed(key_code::left_button)) {
                    move = true;
                    m_operator.focus();
                } else if(!layout_proxy::input_backend().get_key(key_code::left_button))
                    move = false;

                if(move && has_attribute(m_attributes, window_attributes::movable))
                    m_operator.move(layout_proxy::input_backend().mouse_move());

                layout_proxy::add_primitive("background"_id, canvas_fill_rect{ bar, style().background_color });
                if(title.has_value())
                    layout_row(*this, row_alignment::left,
                               [&](row_layout_canvas& canvas) { text(canvas, std::move(title.value())); });
                layout_row(*this, row_alignment::right, [&](row_layout_canvas& canvas) {
                    if(has_attribute(attributes, window_attributes::minimizable)) {
                        if(button_label(canvas, "\u2501"))
                            m_operator.minimize();
                    }
                    if(has_attribute(attributes, window_attributes::maximizable)) {
                        if(button_label(canvas, "\u25A1"))
                            m_operator.maximize();
                    }
                    if(has_attribute(attributes, window_attributes::closable)) {
                        if(button_label(canvas, "X"))
                            m_operator.close();
                    }
                });
                layout_proxy::pop_region(std::nullopt);
                const auto padding = style().padding;
                layout_proxy::push_region(
                    "content"_id, animgui::bounds{ padding.x, m_size.x - padding.x, height + padding.y, m_size.y - padding.y });
            }
        }
        void close() override {
            m_operator.close();
        }
        void focus() override {
            m_operator.focus();
        }
        void finish() {
            // content
            if(!has_attribute(m_attributes, window_attributes::no_title_bar))
                pop_region(std::nullopt);
            // window
            pop_region(std::nullopt);
        }
    };

    class native_window_operator final : public window_operator {
        input_backend& m_input_backend;
        vec2 m_accumulate_delta;

    public:
        explicit native_window_operator(input_backend& input_backend)
            : m_input_backend{ input_backend }, m_accumulate_delta{ 0.0f, 0.0f } {}
        void close() override {
            m_input_backend.close_window();
        }
        void minimize() override {
            m_input_backend.minimize_window();
        }
        void maximize() override {
            m_input_backend.maximize_window();
        }
        void move(const vec2 delta) override {
            m_accumulate_delta.x += delta.x;
            m_accumulate_delta.y += delta.y;
            if(std::fabs(m_accumulate_delta.x) > 1.0f || std::fabs(m_accumulate_delta.y) > 1.0f) {
                const auto dx = std::copysign(std::floor(std::fabs(m_accumulate_delta.x)), m_accumulate_delta.x);
                const auto dy = std::copysign(std::floor(std::fabs(m_accumulate_delta.y)), m_accumulate_delta.y);
                m_accumulate_delta.x -= dx;
                m_accumulate_delta.y -= dy;
                m_input_backend.move_window(static_cast<int32_t>(dx), static_cast<int32_t>(dy));
            }
        }
        void focus() override {
            m_input_backend.focus_window();
        }
    };

    ANIMGUI_API void single_window(canvas& parent, std::optional<std::pmr::string> title, const window_attributes attributes,
                                   const std::function<void(window_canvas&)>& render_function) {
        native_window_operator operator_{ parent.input_backend() };
        const auto [x, y] = parent.reserved_size();
        window_canvas_impl canvas{ parent, { 0.0f, x, 0.0f, y }, std::move(title), attributes, operator_ };
        render_function(canvas);
        canvas.finish();
    }

    class multiple_window_canvas_impl;

    class multiple_window_operator final : public window_operator {
        multiple_window_canvas_impl& m_canvas;
        uid m_id;

    public:
        explicit multiple_window_operator(multiple_window_canvas_impl& canvas, const uid id) : m_canvas{ canvas }, m_id{ id } {}
        void close() override;
        void minimize() override {}
        void maximize() override {}
        void move(vec2 delta) override;
        void focus() override;
    };

    struct windows_info final {
        uid id;
        bounds bounds;
        bool is_open;
        bool auto_adjust;
        windows_info() = delete;
    };

    class multiple_window_canvas_impl final : public multiple_window_canvas {
        std::pmr::unordered_map<uid, std::pair<size_t, size_t>, uid_hasher> m_ranges;
        std::pmr::vector<uid> m_focus_requests;
        std::pmr::vector<uid> m_open_requests;
        std::pmr::vector<uid> m_close_requests;
        std::pmr::vector<std::pair<uid, vec2>> m_move_requests;
        std::pmr::vector<windows_info>& m_info;

        [[nodiscard]] bounds default_bounds() const {
            const auto size = reserved_size();
            const auto cnt = static_cast<float>(m_info.size());
            return { cnt * style().spacing.x, size.x, cnt * style().spacing.y, size.y };
        }

        [[nodiscard]] windows_info& locate_window(const uid id) const {
            for(auto&& win : m_info)
                if(win.id == id)
                    return win;
            m_info.push_back({ id, default_bounds(), true, true });
            return m_info.front();
        }

    public:
        explicit multiple_window_canvas_impl(canvas& parent)
            : multiple_window_canvas{ parent }, m_ranges{ parent.memory_resource() },
              m_focus_requests{ parent.memory_resource() }, m_open_requests{ parent.memory_resource() },
              m_close_requests{ parent.memory_resource() }, m_move_requests{ parent.memory_resource() }, m_info{
                  parent.storage<std::decay_t<decltype(m_info)>>(parent.region_sub_uid())
              } {}
        void new_window(const uid id, std::optional<std::pmr::string> title, const window_attributes attributes,
                        const std::function<void(window_canvas&)>& render_function) override {
            if(auto& [_, bounds, is_open, auto_adjust] = locate_window(id); is_open) {
                multiple_window_operator operator_{ *this, id };
                const auto beg = commands().size();

                const auto size = reserved_size();
                push_region(id, animgui::bounds{ 0.0f, size.x, 0.0f, size.y });
                window_canvas_impl canvas{ *this, bounds, std::move(title), attributes, operator_ };
                render_function(canvas);
                canvas.finish();
                pop_region(std::nullopt);

                const auto end = commands().size();
                m_ranges[id] = { beg, end };
                if(auto_adjust) {
                    std::pmr::vector<vec2> offset{ { { 0.0f, 0.0f } }, memory_resource() };
                    animgui::bounds content_bounds{ 0.0f, 0.0f, 0.0f, 0.0f };
                    for(size_t idx = beg; idx < end; ++idx) {
                        // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
                        switch(const auto& cmd = commands()[idx]; cmd.index()) {
                            case 0: {
                                const auto& sub_bounds = std::get<animgui::push_region>(cmd).bounds;
                                const auto current = offset.back();
                                content_bounds.left = std::fmin(content_bounds.left, sub_bounds.left + current.x);
                                content_bounds.right = std::fmax(content_bounds.right, sub_bounds.right + current.x);
                                content_bounds.top = std::fmin(content_bounds.top, sub_bounds.top + current.y);
                                content_bounds.bottom = std::fmax(content_bounds.bottom, sub_bounds.bottom + current.y);
                                offset.push_back({ current.x + sub_bounds.left, current.y + sub_bounds.top });
                            } break;
                            case 1: {
                                offset.pop_back();
                            } break;
                        }
                    }
                    bounds.right = bounds.left + content_bounds.right - content_bounds.left;
                    bounds.bottom = bounds.top + content_bounds.bottom - content_bounds.top;
                    auto_adjust = false;
                }
            }
        }
        void close_window(const uid id) override {
            m_close_requests.push_back(id);
        }
        void move_window(const uid id, const vec2 delta) {
            m_move_requests.push_back({ id, delta });
        }
        void open_window(const uid id) override {
            m_open_requests.push_back(id);
            m_focus_requests.push_back(id);
        }
        void focus_window(const uid id) override {
            m_focus_requests.push_back(id);
        }
        void finish() {
            for(auto&& id : m_focus_requests) {
                for(auto iter = m_info.begin(); iter != m_info.end(); ++iter) {
                    if(iter->id == id) {
                        while(true) {
                            const auto next = std::next(iter);
                            if(next == m_info.end())
                                break;
                            std::iter_swap(next, iter);
                            iter = next;
                        }
                        break;
                    }
                }
            }
            for(auto&& id : m_close_requests) {
                for(auto& [uid, bounds, is_open, auto_adjust] : m_info)
                    if(id == uid) {
                        is_open = false;
                        break;
                    }
            }
            for(auto&& id : m_open_requests) {
                for(auto& [uid, bounds, is_open, auto_adjust] : m_info)
                    if(id == uid) {
                        is_open = true;
                        break;
                    }
            }
            for(auto&& [id, delta] : m_move_requests) {
                for(auto& [uid, bounds, is_open, auto_adjust] : m_info)
                    if(id == uid) {
                        bounds.left += delta.x;
                        bounds.right += delta.x;
                        bounds.top += delta.y;
                        bounds.bottom += delta.y;
                        break;
                    }
            }

            const auto commands_range = commands();
            std::pmr::vector<operation> new_commands;
            new_commands.reserve(commands_range.size());
            for(auto [id, bounds, is_open, _] : m_info) {
                if(const auto iter = m_ranges.find(id); iter != m_ranges.cend()) {
                    const auto [beg, end] = iter->second;
                    new_commands.insert(new_commands.cend(), commands_range.begin() + beg, commands_range.begin() + end);
                }
            }
            std::move(new_commands.begin(), new_commands.end(), commands_range.begin());
        }
    };

    void multiple_window_operator::close() {
        m_canvas.close_window(m_id);
    }

    void multiple_window_operator::focus() {
        m_canvas.focus_window(m_id);
    }

    void multiple_window_operator::move(const vec2 delta) {
        m_canvas.move_window(m_id, delta);
    }

    ANIMGUI_API void multiple_window(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function) {
        multiple_window_canvas_impl canvas{ parent };
        render_function(canvas);
        canvas.finish();
    }
}  // namespace animgui
