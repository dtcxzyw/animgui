// SPDX-License-Identifier: MIT

#include "../application.hpp"
#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/statistics.hpp>
#include <animgui/core/style.hpp>
#include <string>

namespace animgui {
    class demo final : public application {
        uint32_t m_count = 0;
        std::pmr::string m_text;
        bool m_checkbox_state = false;
        size_t m_index = 0;
        int32_t m_int_value = 0;
        float m_float_value = 0.0f;
        bool m_switch_state = false;

        const pipeline_statistics& m_statistics;

    public:
        explicit demo(context& context) : m_statistics{ context.statistics() } {
            auto&& style = context.global_style();
#if defined(ANIMGUI_WINDOWS)
            style.default_font = context.load_font("msyh", 30.0f);
#elif defined(ANIMGUI_LINUX)
            style.default_font = context.load_font("/nix/store/sksvv8dacm5k66hinhyhlckbjqx1s351-source-sans-pro-3.006/share/fonts/truetype/SourceSansPro-Regular.ttf", 30.0f);
#elif defined(ANIMGUI_MACOS)
            style.default_font = context.load_font("STHeiti Light", 30.0f);
#endif
        }
        void game_pad_content(row_layout_canvas& layout) const {
            for(auto i = 0; i < 5; ++i) {
                for(auto j = 0; j < 5; ++j)
                    button_label(layout, "B");
                layout.newline();
            }

            auto&& input_backend = layout.input();
            for(auto idx : input_backend.list_game_pad()) {
                layout.newline();

                text(layout, std::pmr::string{ std::to_string(idx) + " " } + input_backend.get_game_pad_name(idx));
                layout.newline();
                auto&& state = input_backend.get_game_pad_state(idx);

                text(layout,
                     std::pmr::string{ "leftX: " + std::to_string(state.left_axis.x) +
                                       " leftY: " + std::to_string(state.left_axis.y) });
                layout.newline();
                text(layout,
                     std::pmr::string{ "rightX: " + std::to_string(state.right_axis.x) +
                                       " rightY: " + std::to_string(state.right_axis.y) });
                layout.newline();
                text(layout,
                     std::pmr::string{ "left trigger: " + std::to_string(state.left_trigger) +
                                       " right trigger: " + std::to_string(state.right_trigger) });
                layout.newline();

                auto show_status = [&](const std::string& str, const bool status) {
                    text(layout, std::pmr::string{ str + (status ? " Down" : " Up") });
                };
                show_status("A", state.a);
                show_status("B", state.b);
                show_status("X", state.x);
                show_status("Y", state.y);
                layout.newline();
                show_status("L bumper", state.left_bumper);
                show_status("R bumper", state.right_bumper);
                show_status("L thumb", state.left_thumb);
                show_status("R thumb", state.right_thumb);
                layout.newline();
                show_status("back", state.back);
                show_status("start", state.start);
                show_status("guide", state.guide);
                layout.newline();
                show_status("D-pad up", state.d_pad_up);
                show_status("D-pad right", state.d_pad_right);
                show_status("D-pad down", state.d_pad_down);
                show_status("D-pad left", state.d_pad_left);
                layout.newline();
            }
        }
        void game_pad_window(multiple_window_canvas& manager) {
            manager.new_window(
                "game_pad"_id, "Game Pad ", window_attributes::movable | window_attributes::closable, [&](window_canvas& window) {
                    panel(window, window.region_bounds().size(), scroll_attributes::vertical_scroll, [&](canvas& panel_canvas) {
                        return layout_row(panel_canvas, row_alignment::middle,
                                          [&](row_layout_canvas& layout) { game_pad_content(layout); });
                    });
                });
        }

        void show_frame_info(row_layout_canvas& layout) const {
            text(layout, "Hello World 你好 世界");
            layout.newline();

            text(layout, std::pmr::string{ "FPS " + std::to_string(m_statistics.smooth_fps) });
            layout.newline();
            text(layout,
                 std::pmr::string{ "frame time " + std::to_string(static_cast<float>(m_statistics.frame_time) / 1000.0f) });
            layout.newline();
            text(layout,
                 std::pmr::string{ "input time " + std::to_string(static_cast<float>(m_statistics.input_time) / 1000.0f) });
            layout.newline();
            text(layout, std::pmr::string{ "draw time " + std::to_string(static_cast<float>(m_statistics.draw_time) / 1000.0f) });
            layout.newline();
            text(layout, std::pmr::string{ "emit time " + std::to_string(static_cast<float>(m_statistics.emit_time) / 1000.0f) });
            layout.newline();
            text(layout,
                 std::pmr::string{ "fallback time " + std::to_string(static_cast<float>(m_statistics.fallback_time) / 1000.0f) });
            layout.newline();
            text(layout,
                 std::pmr::string{ "optimize time " + std::to_string(static_cast<float>(m_statistics.optimize_time) / 1000.0f) });
            layout.newline();
            text(layout,
                 std::pmr::string{ "render time " + std::to_string(static_cast<float>(m_statistics.render_time) / 1000.0f) });
            layout.newline();
            text(layout, std::pmr::string{ "generated operation " + std::to_string(m_statistics.generated_operation) });
            layout.newline();
            text(layout, std::pmr::string{ "emitted draw call " + std::to_string(m_statistics.emitted_draw_call) });
            layout.newline();
            text(layout, std::pmr::string{ "transformed draw call " + std::to_string(m_statistics.transformed_draw_call) });
            layout.newline();
            text(layout, std::pmr::string{ "optimized draw call " + std::to_string(m_statistics.optimized_draw_call) });

            layout.newline();
        }

        void main_window(multiple_window_canvas& manager) {
            manager.new_window("base"_id, "Test", window_attributes::movable, [&](window_canvas& window) {
                panel(window, window.region_bounds().size(), scroll_attributes::vertical_scroll, [&](canvas& panel_canvas) {
                    return layout_row(panel_canvas, row_alignment::middle, [&](row_layout_canvas& layout) {
                        show_frame_info(layout);

                        const auto [x, y] = layout.input().get_cursor_pos();
                        text(layout, std::pmr::string{ "X: " + std::to_string(x) + " Y: " + std::to_string(y) });
                        layout.newline();
                        text(layout, "Click: " + std::pmr::string{ std::to_string(m_count) });
                        if(button_label(layout, "Add")) {
                            ++m_count;
                        }
                        layout.newline();
                        text_edit(layout, 20.0f, m_text, "input");
                        layout.newline();
                        checkbox(layout, "checkbox", m_checkbox_state);
                        layout.newline();
                        using clock = std::chrono::high_resolution_clock;
                        constexpr auto second = clock::period::den;
                        progressbar(layout, 300.0f,
                                    static_cast<float>(clock::now().time_since_epoch().count() % second) /
                                        static_cast<float>(second),
                                    std::nullopt);
                        layout.newline();
                        radio_button(layout, { "easy", "normal", "hard" }, m_index);
                        layout.newline();
                        text(layout, std::pmr::string{ "value: " + std::to_string(m_int_value) });
                        slider(layout, 300.0f, 20.0f, m_int_value, 0, 10);
                        layout.newline();
                        text(layout, std::pmr::string{ "value: " + std::to_string(m_float_value) });
                        slider(layout, 300.0f, 20.0f, m_float_value, 0.0f, 10.0f);
                        layout.newline();
                        text(layout, "switch");
                        switch_(layout, m_switch_state);
                        layout.newline();
                        if(button_label(layout, "game pad")) {
                            manager.open_window("game_pad"_id);
                        }
                        layout.newline();
                        if(button_label(layout, "Exit")) {
                            layout.input().close_window();
                        }

                        for(uint32_t i = 0; i < 50; ++i) {
                            layout.newline();
                            text(layout, std::pmr::string{ "scroll test " + std::to_string(i) });
                        }
                    });
                });
            });
        }

        void render(canvas& canvas_root) override {
            single_window(canvas_root, "Test",
                          window_attributes::closable | window_attributes::minimizable | window_attributes::maximizable |
                              window_attributes::movable,
                          [&](window_canvas& full) {
                              multiple_window(full, [&](multiple_window_canvas& manager) {
                                  main_window(manager);
                                  game_pad_window(manager);
                              });
                          });
        }
    };

    std::shared_ptr<application> create_demo_application(context& context) {
        return std::make_shared<demo>(context);
    }
}  // namespace animgui
