// SPDX-License-Identifier: MIT

#pragma once

#include "common.hpp"
#include "render_backend.hpp"

#include <functional>
#include <memory>
#include <memory_resource>

namespace animgui {
    class input_backend;
    class render_backend;
    class font_backend;
    struct style;
    class animator;
    class emitter;
    class canvas;

    // TODO: color management & interacting mode(mouse&keyboard/VR/game pad)
    // TODO: export image_compactor & command optimizer
    class context {
    public:
        context() = default;
        virtual ~context() = default;
        context(const context& rhs) = delete;
        context(context&& rhs) = default;
        context& operator=(const context& rhs) = delete;
        context& operator=(context&& rhs) = default;

        virtual void new_frame(uint32_t width, uint32_t height, float delta_t,
                               const std::function<void(canvas& canvas)>& render_function) = 0;
        virtual void reset_cache() = 0;
        virtual texture_region load_image(const image_desc& image) = 0;
        virtual style& style() noexcept = 0;
    };

    std::unique_ptr<context> create_animgui_context(input_backend& input_backend, font_backend& font_backend,
                                                    render_backend& render_backend, emitter& emitter, animator& animator,
                                                    std::pmr::memory_resource* memory_manager = std::pmr::get_default_resource());

};  // namespace animgui
