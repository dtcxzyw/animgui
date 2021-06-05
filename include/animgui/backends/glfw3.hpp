// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <memory>
struct GLFWwindow;

namespace animgui {
    class input_backend;
    ANIMGUI_API std::shared_ptr<input_backend> create_glfw3_backend(GLFWwindow* window);
}  // namespace animgui
