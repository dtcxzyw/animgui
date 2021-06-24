// SPDX-License-Identifier: MIT

#pragma once

#include "render_backend.hpp"
#include <functional>
#include <memory>

namespace animgui {
    class input_backend;
    class render_backend;
    class font_backend;
    struct style;
    class animator;
    class emitter;
    class canvas;
    class command_optimizer;
    class image_compactor;
    class font;

    // TODO: color management & interacting mode(mouse&keyboard/VR/game pad)
    class context {
    public:
        context() = default;
        virtual ~context() = default;
        context(const context& rhs) = delete;
        context(context&& rhs) = default;
        context& operator=(const context& rhs) = delete;
        context& operator=(context&& rhs) = default;

        virtual void new_frame(uint32_t width, uint32_t height, float delta_t,
                               const std::function<void(canvas&)>& render_function) = 0;
        virtual void reset_cache() = 0;
        virtual texture_region load_image(const image_desc& image, float max_scale) = 0;
        [[nodiscard]] virtual std::shared_ptr<font> load_font(const std::pmr::string& name, float height) const = 0;
        virtual style& global_style() noexcept = 0;
    };

    ANIMGUI_API std::unique_ptr<context>
    create_animgui_context(input_backend& input_backend, render_backend& render_backend, font_backend& font_backend,
                           emitter& emitter, animator& animator, command_optimizer& command_optimizer,
                           image_compactor& image_compactor,
                           std::pmr::memory_resource* memory_manager = std::pmr::get_default_resource());

};  // namespace animgui
