// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <memory>

namespace animgui {
    class render_backend;
    ANIMGUI_API std::shared_ptr<render_backend> create_opengl3_backend();
}  // namespace animgui
