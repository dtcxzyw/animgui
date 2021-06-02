// SPDX-License-Identifier: MIT

#pragma once
#include <functional>

namespace animgui {
    class canvas;
    class context;
    void set_classic_style(context& context);
    void styled(canvas& context, std::function<std::function<void()>()> modifier, std::function<void(canvas&)> render_function);
}  // namespace animgui
