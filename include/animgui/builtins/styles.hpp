// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>

namespace animgui {
    class context;
    ANIMGUI_API void set_classic_style(context& context);

    ANIMGUI_API bool validate_style_accessibility(context& context);
}  // namespace animgui
