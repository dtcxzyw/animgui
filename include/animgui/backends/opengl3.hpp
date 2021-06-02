// SPDX-License-Identifier: MIT

#pragma once
#include <memory>

namespace animgui {
    class render_backend;
    std::unique_ptr<render_backend> create_opengl3_backend();
}  // namespace animgui
