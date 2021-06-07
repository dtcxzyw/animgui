// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <functional>
#include <memory>
struct GLFWwindow;

namespace animgui {
    class input_backend;
    ANIMGUI_API std::shared_ptr<input_backend> create_glfw3_backend(GLFWwindow* window, const std::function<void()>& redraw);
}  // namespace animgui
