// SPDX-License-Identifier: MIT

#include <GLFW/glfw3.h>
#include <animgui/backends/glfw3.hpp>
#include <animgui/core/input_backend.hpp>

namespace animgui {
    static key_code cast_key_code(int code) {
        switch(code) {
            case GLFW_MOUSE_BUTTON_LEFT:
                return key_code::left_button;
            case GLFW_MOUSE_BUTTON_RIGHT:
                return key_code::right_button;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                return key_code::middle_button;
            case GLFW_KEY_SPACE:
                return key_code::space;
            case GLFW_KEY_APOSTROPHE:
                return key_code::apostrophe;
            case GLFW_KEY_COMMA:
                return key_code::comma;
            case GLFW_KEY_MINUS:
                return key_code::minus;
            case GLFW_KEY_PERIOD:
                return key_code::period;
            case GLFW_KEY_SLASH:
                return key_code::slash;
            case GLFW_KEY_0:  // NOLINT(bugprone-branch-clone)
                [[fallthrough]];
            case GLFW_KEY_1:
                [[fallthrough]];
            case GLFW_KEY_2:
                [[fallthrough]];
            case GLFW_KEY_3:
                [[fallthrough]];
            case GLFW_KEY_4:
                [[fallthrough]];
            case GLFW_KEY_5:
                [[fallthrough]];
            case GLFW_KEY_6:
                [[fallthrough]];
            case GLFW_KEY_7:
                [[fallthrough]];
            case GLFW_KEY_8:
                [[fallthrough]];
            case GLFW_KEY_9:
                return static_cast<key_code>(code);
            case GLFW_KEY_SEMICOLON:
                return key_code::semicolon;
            case GLFW_KEY_EQUAL:
                return key_code::equal;
            case GLFW_KEY_A:  // NOLINT(bugprone-branch-clone)
                [[fallthrough]];
            case GLFW_KEY_B:
                [[fallthrough]];
            case GLFW_KEY_C:
                [[fallthrough]];
            case GLFW_KEY_D:
                [[fallthrough]];
            case GLFW_KEY_E:
                [[fallthrough]];
            case GLFW_KEY_F:
                [[fallthrough]];
            case GLFW_KEY_G:
                [[fallthrough]];
            case GLFW_KEY_H:
                [[fallthrough]];
            case GLFW_KEY_I:
                [[fallthrough]];
            case GLFW_KEY_J:
                [[fallthrough]];
            case GLFW_KEY_K:
                [[fallthrough]];
            case GLFW_KEY_L:
                [[fallthrough]];
            case GLFW_KEY_M:
                [[fallthrough]];
            case GLFW_KEY_N:
                [[fallthrough]];
            case GLFW_KEY_O:
                [[fallthrough]];
            case GLFW_KEY_P:
                [[fallthrough]];
            case GLFW_KEY_Q:
                [[fallthrough]];
            case GLFW_KEY_R:
                [[fallthrough]];
            case GLFW_KEY_S:
                [[fallthrough]];
            case GLFW_KEY_T:
                [[fallthrough]];
            case GLFW_KEY_U:
                [[fallthrough]];
            case GLFW_KEY_V:
                [[fallthrough]];
            case GLFW_KEY_W:
                [[fallthrough]];
            case GLFW_KEY_X:
                [[fallthrough]];
            case GLFW_KEY_Y:
                [[fallthrough]];
            case GLFW_KEY_Z:
                return static_cast<key_code>(code);
            case GLFW_KEY_LEFT_BRACKET:
                return key_code ::left_bracket;
            case GLFW_KEY_BACKSLASH:
                return key_code ::backslash;
            case GLFW_KEY_RIGHT_BRACKET:
                return key_code ::right_bracket;
            case GLFW_KEY_GRAVE_ACCENT:
                return key_code::grave_accent;
            case GLFW_KEY_ESCAPE:
                return key_code ::escape;
            case GLFW_KEY_ENTER:
                return key_code::enter;
            case GLFW_KEY_TAB:
                return key_code::tab;
            case GLFW_KEY_BACKSPACE:
                return key_code ::back;
            case GLFW_KEY_INSERT:
                return key_code ::insert;
            case GLFW_KEY_DELETE:
                return key_code ::delete_;
            case GLFW_KEY_RIGHT:
                return key_code ::right;
            case GLFW_KEY_LEFT:
                return key_code ::left;
            case GLFW_KEY_DOWN:
                return key_code ::down;
            case GLFW_KEY_UP:
                return key_code ::up;
            case GLFW_KEY_PAGE_UP:
                return key_code ::page_up;
            case GLFW_KEY_PAGE_DOWN:
                return key_code ::page_down;
            case GLFW_KEY_HOME:
                return key_code ::home;
            case GLFW_KEY_END:
                return key_code ::end;
            case GLFW_KEY_CAPS_LOCK:
                return key_code ::capital;
            case GLFW_KEY_PAUSE:
                return key_code ::pause;
            case GLFW_KEY_F1:  // NOLINT(bugprone-branch-clone)
                [[fallthrough]];
            case GLFW_KEY_F2:
                [[fallthrough]];
            case GLFW_KEY_F3:
                [[fallthrough]];
            case GLFW_KEY_F4:
                [[fallthrough]];
            case GLFW_KEY_F5:
                [[fallthrough]];
            case GLFW_KEY_F6:
                [[fallthrough]];
            case GLFW_KEY_F7:
                [[fallthrough]];
            case GLFW_KEY_F8:
                [[fallthrough]];
            case GLFW_KEY_F9:
                [[fallthrough]];
            case GLFW_KEY_F10:
                [[fallthrough]];
            case GLFW_KEY_F11:
                [[fallthrough]];
            case GLFW_KEY_F12:
                return static_cast<key_code>(code - GLFW_KEY_F1 + static_cast<int>(key_code::f1));

            case GLFW_KEY_KP_0:  // NOLINT(bugprone-branch-clone)
                [[fallthrough]];
            case GLFW_KEY_KP_1:
                [[fallthrough]];
            case GLFW_KEY_KP_2:
                [[fallthrough]];
            case GLFW_KEY_KP_3:
                [[fallthrough]];
            case GLFW_KEY_KP_4:
                [[fallthrough]];
            case GLFW_KEY_KP_5:
                [[fallthrough]];
            case GLFW_KEY_KP_6:
                [[fallthrough]];
            case GLFW_KEY_KP_7:
                [[fallthrough]];
            case GLFW_KEY_KP_8:
                [[fallthrough]];
            case GLFW_KEY_KP_9:
                return static_cast<key_code>(code - GLFW_KEY_KP_0 + '0');
            case GLFW_KEY_KP_ENTER:
                return key_code ::enter;
            case GLFW_KEY_LEFT_SHIFT:
                return key_code::left_shift;
            case GLFW_KEY_LEFT_CONTROL:
                return key_code ::left_control;
            case GLFW_KEY_LEFT_ALT:
                return key_code ::left_alt;
            case GLFW_KEY_RIGHT_SHIFT:
                return key_code ::right_shift;
            case GLFW_KEY_RIGHT_CONTROL:
                return key_code ::right_control;
            case GLFW_KEY_RIGHT_ALT:
                return key_code::right_alt;
            case GLFW_KEY_MENU:
                return key_code ::menu;
            default:
                return key_code::max;
        }
    }
    class glfw3_backend final : public input_backend {
        static constexpr auto game_pad_axis_eps = 0.08f;

        GLFWwindow* m_window;
        vec2 m_cursor_pos;
        vec2 m_mouse_move;
        vec2 m_scroll;
        bool m_key_state[256];
        input_mode m_input_mode;
        cursor m_cursor;
        std::unordered_map<cursor, GLFWcursor*> m_cursors;

        game_pad_state m_game_pad_state[GLFW_JOYSTICK_LAST + 1];
        std::vector<size_t> m_available_game_pad;

        const std::function<void()>& m_redraw;

        void add_char(const uint32_t codepoint) {}
        void key_event(const int key, const bool state) {
            m_key_state[static_cast<uint32_t>(cast_key_code(key))] = state;
        }
        void cursor_event(const vec2 pos) {
            m_input_mode = input_mode::mouse;
            m_mouse_move.x += pos.x - m_cursor_pos.x;
            m_mouse_move.y += pos.y - m_cursor_pos.y;
            m_cursor_pos = pos;
        }
        void scroll_event(const vec2 offset) {
            m_input_mode = input_mode::mouse;
            m_scroll.x += offset.x;
            m_scroll.y += offset.y;
        }
        void refresh_event() const {
            m_redraw();
        }

    public:
        explicit glfw3_backend(GLFWwindow* window, const std::function<void()>& redraw)
            : m_window{ window }, m_cursor_pos{ 0.0f, 0.0f }, m_mouse_move{ 0.0f, 0.0f }, m_scroll{ 0.0f, 0.0f }, m_key_state{},
              m_input_mode{ input_mode::mouse }, m_cursor{ cursor::arrow }, m_game_pad_state{}, m_redraw{ redraw } {
            m_available_game_pad.reserve(std::size(m_game_pad_state));
            glfwSetWindowUserPointer(m_window, this);
            glfwSetCharCallback(m_window, [](GLFWwindow* const win, const unsigned int cp) {
                static_cast<glfw3_backend*>(glfwGetWindowUserPointer(win))->add_char(cp);
            });
            // TODO: modifier keys?
            glfwSetKeyCallback(m_window, [](GLFWwindow* const win, const int key, int, const int action, int) {
                static_cast<glfw3_backend*>(glfwGetWindowUserPointer(win))->key_event(key, action != GLFW_RELEASE);
            });
            glfwSetMouseButtonCallback(m_window, [](GLFWwindow* const win, const int key, const int action, int) {
                static_cast<glfw3_backend*>(glfwGetWindowUserPointer(win))->key_event(key, action != GLFW_RELEASE);
            });
            glfwSetScrollCallback(m_window, [](GLFWwindow* const win, const double x, const double y) {
                static_cast<glfw3_backend*>(glfwGetWindowUserPointer(win))
                    ->scroll_event(vec2{ static_cast<float>(x), static_cast<float>(y) });
            });
            glfwSetCursorPosCallback(m_window, [](GLFWwindow* const win, const double x, const double y) {
                static_cast<glfw3_backend*>(glfwGetWindowUserPointer(win))
                    ->cursor_event(vec2{ static_cast<float>(x), static_cast<float>(y) });
            });
            glfwSetWindowRefreshCallback(m_window, [](GLFWwindow* const win) {
                static_cast<glfw3_backend*>(glfwGetWindowUserPointer(win))->refresh_event();
            });
            m_cursors[cursor::arrow] = nullptr;
            m_cursors[cursor::cross_hair] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
            m_cursors[cursor::edit] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
            m_cursors[cursor::hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
            m_cursors[cursor::horizontal] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
            m_cursors[cursor::vertical] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        }

        glfw3_backend(const glfw3_backend&) = delete;
        glfw3_backend(glfw3_backend&&) = delete;
        glfw3_backend& operator=(const glfw3_backend&) = delete;
        glfw3_backend& operator=(glfw3_backend&&) = delete;
        ~glfw3_backend() override {
            for(auto [_, cursor] : m_cursors)
                glfwDestroyCursor(cursor);
        }

        std::pmr::string get_clipboard_text() override {
            return glfwGetClipboardString(nullptr);
        }
        vec2 get_cursor_pos() override {
            return m_cursor_pos;
        }
        bool get_key(key_code code) override {
            return m_key_state[static_cast<uint32_t>(code)];
        }
        void set_clipboard_text(const std::pmr::string& str) override {
            glfwSetClipboardString(nullptr, str.c_str());
        }
        void close_window() override {
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }
        void minimize_window() override {
            glfwIconifyWindow(m_window);
        }
        void maximize_window() override {
            if(glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED) == GLFW_TRUE)
                glfwRestoreWindow(m_window);
            else
                glfwMaximizeWindow(m_window);
        }
        void move_window(const int32_t dx, const int32_t dy) override {
            int32_t x, y;
            glfwGetWindowPos(m_window, &x, &y);
            x += dx;
            y += dy;
            m_mouse_move = { 0.0f, 0.0f };
            m_cursor_pos.x -= static_cast<float>(dx);
            m_cursor_pos.y -= static_cast<float>(dy);
            glfwSetWindowPos(m_window, x, y);
        }
        [[nodiscard]] vec2 mouse_move() const noexcept override {
            return m_mouse_move;
        }
        [[nodiscard]] vec2 scroll() const noexcept override {
            return m_scroll;
        }
        void new_frame() override {
            glfwSetCursor(m_window, m_cursors[m_cursor]);

            m_cursor = cursor::arrow;
            m_mouse_move = m_scroll = { 0.0f, 0.0f };
            m_available_game_pad.clear();
            const auto flush = [](float& x) { x = std::fabs(x) < game_pad_axis_eps ? 0.0f : x; };
            for(int i = 0; i <= GLFW_JOYSTICK_LAST; ++i) {
                if(!glfwJoystickPresent(i) || !glfwJoystickIsGamepad(i))
                    continue;
                // TODO: hats
                static_assert(sizeof(game_pad_state) == sizeof(GLFWgamepadstate));
                if(auto state = reinterpret_cast<GLFWgamepadstate*>(&m_game_pad_state[i]); glfwGetGamepadState(i, state)) {
                    for(int j = 0; j < 4; ++j)
                        flush(state->axes[j]);
                    m_available_game_pad.push_back(i);
                    if(m_input_mode != input_mode::game_pad) {
                        for(auto&& button : state->buttons)
                            if(button == GLFW_PRESS) {
                                m_input_mode = input_mode::game_pad;
                                break;
                            }
                    }
                    if(m_input_mode != input_mode::game_pad) {
                        if(state->axes[0] != 0.0f || state->axes[1] != 0.0f || state->axes[2] != 0.0f || state->axes[3] != 0.0f ||
                           std::fabs(state->axes[4] - -1.0f) > game_pad_axis_eps ||
                           std::fabs(state->axes[5] - -1.0f) > game_pad_axis_eps)
                            m_input_mode = input_mode::game_pad;
                    }
                }
            }
        }
        [[nodiscard]] std::pmr::string get_game_pad_name(const size_t idx) const override {
            return glfwGetGamepadName(static_cast<int>(idx));
        }
        [[nodiscard]] const game_pad_state& get_game_pad_state(const size_t idx) const noexcept override {
            return m_game_pad_state[idx];
        }
        [[nodiscard]] span<const size_t> list_game_pad() const noexcept override {
            return { m_available_game_pad.data(), m_available_game_pad.data() + m_available_game_pad.size() };
        }
        [[nodiscard]] input_mode get_input_mode() const noexcept override {
            return m_input_mode;
        }
        void set_cursor(const cursor cursor) noexcept override {
            if(m_cursor == cursor::arrow)
                m_cursor = cursor;
        }
        void focus_window() override {
            glfwFocusWindow(m_window);
        }
    };

    ANIMGUI_API std::shared_ptr<input_backend> create_glfw3_backend(GLFWwindow* window, const std::function<void()>& redraw) {
        return std::make_shared<glfw3_backend>(window, redraw);
    }
}  // namespace animgui
