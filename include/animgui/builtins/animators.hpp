// SPDX-License-Identifier: MIT

#pragma once
#include <memory>

namespace animgui {
    class animator;
    std::unique_ptr<animator> create_dummy_animator();
    std::unique_ptr<animator> create_linear_animator(float speed);
    std::unique_ptr<animator> create_physical_animator(float speed);
}  // namespace animgui
