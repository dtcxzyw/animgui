// SPDX-License-Identifier: MIT

#pragma once
#include "render_backend.hpp"
#include <vector>

namespace animgui {
    class command_optimizer {
    public:
        command_optimizer() = default;
        virtual ~command_optimizer() = default;
        command_optimizer(const command_optimizer& rhs) = delete;
        command_optimizer(command_optimizer&& rhs) = default;
        command_optimizer& operator=(const command_optimizer& rhs) = delete;
        command_optimizer& operator=(command_optimizer&& rhs) = default;

        [[nodiscard]] virtual std::pmr::vector<command> optimize(uvec2 size, std::pmr::vector<command> src) const = 0;
        [[nodiscard]] virtual primitive_type supported_primitives() const noexcept = 0;
    };
}  // namespace animgui
