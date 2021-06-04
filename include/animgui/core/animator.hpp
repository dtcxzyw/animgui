// SPDX-License-Identifier: MIT

#pragma once
#include <functional>
#include <utility>

namespace animgui {
    using step_function = std::function<float(float, void*)>;  // value=step(destination,state)

    // TODO: enter & exit
    class animator {
    public:
        animator() = default;
        virtual ~animator() = default;
        animator(const animator& rhs) = delete;
        animator(animator&& rhs) = default;
        animator& operator=(const animator& rhs) = delete;
        animator& operator=(animator&& rhs) = default;

        // hash/size/alignment
        [[nodiscard]] virtual std::tuple<size_t, size_t, size_t> state_storage() const noexcept = 0;
        [[nodiscard]] virtual step_function step(float delta_t) const = 0;
    };
}  // namespace animgui
