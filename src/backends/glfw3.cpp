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
        GLFWwindow* m_window;
        vec2 m_cursor_pos;
        bool m_key_state[256];

        void add_char(const uint32_t codepoint) {}
        void key_event(const int key, const bool state) {
            m_key_state[static_cast<uint32_t>(cast_key_code(key))] = state;
        }
        void cursor_event(const vec2 pos) {
            m_cursor_pos = pos;
        }
        void scroll_event(vec2 offset) {}

    public:
        explicit glfw3_backend(GLFWwindow* window) : m_window{ window }, m_cursor_pos{ 0.0f, 0.0f }, m_key_state{} {
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
    };

    std::shared_ptr<input_backend> create_glfw3_backend(GLFWwindow* window) {
        return std::make_shared<glfw3_backend>(window);
    }
}  // namespace animgui
