// SPDX-License-Identifier: MIT

#include <animgui/builtins/command_optimizers.hpp>
#include <animgui/core/command_optimizer.hpp>
#include <vector>

namespace animgui {
    class command_optimizer_impl final : public command_optimizer {
    public:
        [[nodiscard]] std::pmr::vector<command> optimize(std::pmr::vector<command> src) const override {
            // noop
            return src;
        }
    };

    ANIMGUI_API std::shared_ptr<command_optimizer> create_builtin_command_optimizer() {
        return std::make_shared<command_optimizer_impl>();
    }
}  // namespace animgui
