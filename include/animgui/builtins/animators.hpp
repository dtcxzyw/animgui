// SPDX-License-Identifier: MIT

#pragma once
#include "../core/common.hpp"
#include <memory>

namespace animgui {
    class animator;
    ANIMGUI_API std::shared_ptr<animator> create_dummy_animator();
    ANIMGUI_API std::shared_ptr<animator> create_linear_animator(float speed);
    ANIMGUI_API std::shared_ptr<animator> create_physical_animator(float speed);
}  // namespace animgui
