// SPDX-License-Identifier: MIT

#pragma once
#include <memory>
struct GLFWwindow;

namespace animgui {
    class input_backend;
    std::shared_ptr<input_backend> create_glfw3_backend(GLFWwindow* window);
}  // namespace animgui
