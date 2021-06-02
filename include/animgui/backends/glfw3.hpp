// SPDX-License-Identifier: MIT

#pragma once
#include <memory>
struct GLFWWindow;

namespace animgui {
    class input_backend;
    std::unique_ptr<input_backend> create_glfw3_backend(GLFWWindow* window);
}  // namespace animgui
