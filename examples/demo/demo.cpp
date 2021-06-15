// SPDX-License-Identifier: MIT

#include "../application.hpp"
#include "animgui/core/input_backend.hpp"

#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/style.hpp>
#include <string>

namespace animgui {
    class demo final : public application {
        uint32_t m_count;

    public:
        explicit demo(context& context) : m_count{ 0 } {
            auto&& style = context.style();
            style.font = context.load_font("msyh", 30.0f);
        }
        void test_game_pad(row_layout_canvas& canvas) const {
            auto&& input_backend = canvas.input_backend();
            for(auto idx : input_backend.list_game_pad()) {
                text(canvas, std::pmr::string{ std::to_string(idx) + " " } + input_backend.get_game_pad_name(idx));
                canvas.newline();
                auto&& state = input_backend.get_game_pad_state(idx);

                text(canvas,
                     std::pmr::string{ "leftX: " + std::to_string(state.left_axis.x) +
                                       " leftY: " + std::to_string(state.left_axis.y) });
                canvas.newline();
                text(canvas,
                     std::pmr::string{ "rightX: " + std::to_string(state.right_axis.x) +
                                       " rightY: " + std::to_string(state.right_axis.y) });
                canvas.newline();
                text(canvas,
                     std::pmr::string{ "left trigger: " + std::to_string(state.left_trigger) +
                                       " right trigger: " + std::to_string(state.right_trigger) });
                canvas.newline();

                auto show_status = [&](const std::string& str, const bool status) {
                    text(canvas, std::pmr::string{ str + (status ? " Down" : " Up") });
                };
                show_status("A", state.a);
                show_status("B", state.b);
                show_status("X", state.x);
                show_status("Y", state.y);
                canvas.newline();
                show_status("L bumper", state.left_bumper);
                show_status("R bumper", state.right_bumper);
                show_status("L thumb", state.left_thumb);
                show_status("R thumb", state.right_thumb);
                canvas.newline();
                show_status("back", state.back);
                show_status("start", state.start);
                show_status("guide", state.guide);
                canvas.newline();
                show_status("D-pad up", state.d_pad_up);
                show_status("D-pad right", state.d_pad_right);
                show_status("D-pad down", state.d_pad_down);
                show_status("D-pad left", state.d_pad_left);
                canvas.newline();
            }
        }
        void render(canvas& canvas) override {
            single_window(canvas, "Test",
                          window_attributes::closable | window_attributes::minimizable | window_attributes::maximizable |
                              window_attributes::movable,
                          [&](window_canvas& full) {
                              multiple_window(full, [&](multiple_window_canvas& manager) {
                                  manager.new_window("base"_id, "Test", window_attributes::movable, [&](window_canvas& window) {
                                      layout_row_center(window, [&](row_layout_canvas& layout) {
                                          text(layout, "Hello World 你好 世界");
                                          layout.newline();
                                          text(layout, "Click: " + std::pmr::string{ std::to_string(m_count) });
                                          if(button_label(layout, "Add")) {
                                              ++m_count;
                                          }
                                          layout.newline();
                                          if(button_label(layout, "game pad")) {
                                              manager.open_window("game_pad"_id);
                                          }
                                          layout.newline();
                                          if(button_label(layout, "Exit")) {
                                              layout.input_backend().close_window();
                                          }
                                      });
                                  });
                                  manager.new_window("game_pad"_id, "Game Pad",
                                                     window_attributes::movable | window_attributes::closable,
                                                     [&](window_canvas& window) {
                                                         layout_row_center(window, [&](row_layout_canvas& layout) {
                                                             for(auto i = 0; i < 5; ++i) {
                                                                 for(auto j = 0; j < 5; ++j)
                                                                     button_label(layout, "B");
                                                                 layout.newline();
                                                             }

                                                             test_game_pad(layout);
                                                         });
                                                     });
                              });
                          });
        }
    };

    std::shared_ptr<application> create_demo_application(context& context) {
        return std::make_shared<demo>(context);
    }
}  // namespace animgui
