// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <memory>
#include <memory_resource>

namespace animgui {
    class image_compactor;
    class render_backend;

    ANIMGUI_API std::shared_ptr<image_compactor> create_builtin_image_compactor(render_backend& render_backend,
                                                                                std::pmr::memory_resource* memory_resource);
}  // namespace animgui
