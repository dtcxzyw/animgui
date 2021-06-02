// SPDX-License-Identifier: MIT

#pragma once
#include "../core/canvas.hpp"
#include <functional>

namespace animgui {
    class row_layout_canvas : public canvas {
    public:
        virtual void newline() = 0;
    };
    enum class row_alignment { left, right, middle, justify };

    void layout_row(canvas& parent, row_alignment alignment, const std::function<void(row_layout_canvas&)>& render_function);
    void layout_row_center(canvas& parent, const std::function<void(row_layout_canvas&)>& render_function);

    // TODO: menu
    class multiple_window_canvas : public canvas {
    public:
        virtual void new_window(const std::function<void(canvas&)>& render_function);
    };
    void multiple_window(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);
    void docking(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);
}  // namespace animgui
