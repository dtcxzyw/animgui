// SPDX-License-Identifier: MIT

#pragma once
#include <memory>
#include <memory_resource>

namespace animgui {
    class emitter;

    std::unique_ptr<emitter> create_builtin_emitter(std::pmr::memory_resource* memory_resource);
}  // namespace animgui
