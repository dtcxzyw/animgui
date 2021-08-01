// SPDX-License-Identifier: MIT

#pragma once
#include "render_backend.hpp"

namespace animgui {
    class command_optimizer {
    public:
        command_optimizer() = default;
        virtual ~command_optimizer() = default;
        command_optimizer(const command_optimizer& rhs) = delete;
        command_optimizer(command_optimizer&& rhs) = default;
        command_optimizer& operator=(const command_optimizer& rhs) = delete;
        command_optimizer& operator=(command_optimizer&& rhs) = default;

        [[nodiscard]] virtual command_queue optimize(uvec2 size, command_queue src) const = 0;
        [[nodiscard]] virtual primitive_type supported_primitives() const noexcept = 0;
    };
}  // namespace animgui
