// SPDX-License-Identifier: MIT

#pragma once
#include <memory>

namespace animgui {
    class font_backend;
    std::shared_ptr<font_backend> create_stb_font_backend();
}  // namespace animgui
