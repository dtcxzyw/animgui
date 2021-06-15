// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <memory>

namespace animgui {
    class font_backend;
    ANIMGUI_API std::shared_ptr<font_backend> create_stb_font_backend(float super_sample = 1.0f);
}  // namespace animgui
