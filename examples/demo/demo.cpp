// SPDX-License-Identifier: MIT

#include "../application.hpp"
#include <string>
#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/style.hpp>

namespace animgui {
    class demo final : public application {
        uint32_t m_count;

    public:
        explicit demo(context& context) : m_count{ 0 } {
            auto&& style = context.style();
            style.font = context.load_font("msyh", 100.0f);
        }
        void render(canvas& canvas) override {
            animgui::layout_row_center(canvas, [&](animgui::row_layout_canvas& layout) {
                animgui::text(layout, "Hello World 你好 世界");
                layout.newline();
                animgui::text(layout, "Click: " + std::pmr::string{ std::to_string(m_count) });
                if(animgui::button_label(layout, "Add")) {
                    ++m_count;
                }
                layout.newline();
                if(animgui::button_label(layout, "Exit")) {
                    std::exit(0);
                }
            });
        }
    };

    std::shared_ptr<application> create_demo_application(context& context) {
        return std::make_shared<demo>(context);
    }
}  // namespace animgui
