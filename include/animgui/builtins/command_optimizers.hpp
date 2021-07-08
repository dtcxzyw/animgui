// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <memory>

namespace animgui {
    class command_optimizer;

    ANIMGUI_API std::shared_ptr<command_optimizer> create_noop_command_optimizer();
    ANIMGUI_API std::shared_ptr<command_optimizer> create_builtin_command_optimizer();
}  // namespace animgui
