// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <memory>
#include <memory_resource>

namespace animgui {
    class emitter;

    ANIMGUI_API std::shared_ptr<emitter> create_builtin_emitter(std::pmr::memory_resource* memory_resource);
}  // namespace animgui
