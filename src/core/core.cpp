// SPDX-License-Identifier: MIT

#include "animgui/core/animator.hpp"

#include <animgui/core/canvas.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/style.hpp>

namespace animgui {
    class canvas_impl final : public canvas {
        context& m_context;

    public:
        canvas_impl(context& context) : m_context{ context } {}
        animgui::style& style() noexcept override {
            return m_context.style();
        }
        [[nodiscard]] void* storage(size_t hash, uid uid) override {}
        std::pmr::vector<operation>& commands() override {}
        [[nodiscard]] vec2 reserved_size() override {}
        void register_type(size_t hash, callback ctor, callback dtor) override {}
        void pop_region() override {}
        uid push_region(uid uid, const bounds& bounds) override;
        [[nodiscard]] bool pressed(const bounds& bounds) const override;
        [[nodiscard]] bool widget_pressed() const override;
        [[nodiscard]] bounds widget_bounds() const override;
        [[nodiscard]] bool widget_hovered() const override;
        [[nodiscard]] bool hovered(const bounds& bounds) const override;
        void set_cursor(cursor cursor) override;
        uid add_primitive(uid uid, const primitive& primitive) override;
    };
    class command_optimizer final {
    public:
        [[nodiscard]] std::pmr::vector<command> optimize(const std::pmr::vector<command>& src) const {
            return src;
        }
    };
    struct compacted_image final {};
    class image_compactor final {
        std::pmr::vector<compacted_image> m_images;
        render_backend& m_backend;
        static constexpr uint32_t image_pool_size = 1024;

    public:
        explicit image_compactor(render_backend& render_backend, std::pmr::memory_resource* memory_manager)
            : m_images{ memory_manager }, m_backend{ render_backend } {}
        ~image_compactor() = default;
        image_compactor(const image_compactor& rhs) = delete;
        image_compactor(image_compactor&& rhs) = default;
        image_compactor& operator=(const image_compactor& rhs) = delete;
        image_compactor& operator=(image_compactor&& rhs) = delete;

        texture_region compact(const image_desc& image) {}
    };
    class context_impl final : public context {
        input_backend& m_input_backend;
        font_backend& m_font_backend;
        render_backend& m_render_backend;
        emitter& m_emitter;
        animator& m_animator;

        std::pmr::unordered_map<uid, std::pair<size_t, size_t>> m_state_location;
        std::pmr::unordered_map<size_t, std::pmr::vector<std::byte>> m_state_storage;
        image_compactor m_image_compactor;
        command_optimizer m_command_optimizer;
        std::pmr::memory_resource* m_memory_manager;
        animgui::style m_style;

    public:
        context_impl(input_backend& input_backend, font_backend& font_backend, render_backend& render_backend, emitter& emitter,
                     animator& animator, std::pmr::memory_resource* memory_manager)
            : m_input_backend{ input_backend }, m_font_backend{ font_backend }, m_render_backend{ render_backend },
              m_emitter{ emitter }, m_animator{ animator }, m_state_location{ memory_manager }, m_state_storage{ memory_manager },
              m_image_compactor{ render_backend, memory_manager }, m_memory_manager{ memory_manager }, m_style{
                  nullptr,
                  '?',
                  nullptr,
                  { 0.0f, 0.0f, 0.0f, 1.0f },
                  { 1.0f, 1.0f, 1.0f, 1.0f },
                  { 1.0f, 1.0f, 1.0f, 1.0f },
                  { memory_manager }
              } {}
        void reset_cache() override {
            m_state_location.clear();
            m_state_storage.clear();
        }
        animgui::style& style() noexcept override {
            return m_style;
        }
        void new_frame(uint32_t width, uint32_t height, const float delta_t,
                       const std::function<void(canvas& canvas)>& render_function) override {
            std::pmr::monotonic_buffer_resource arena{ 1 << 15, m_memory_manager };
            canvas_impl canvas{};
            auto step = m_animator.step(delta_t);
            render_function(canvas);
            const auto commands = m_emitter.transform(canvas.commands());
            const auto optimized_commands = m_command_optimizer.optimize(commands);
            m_render_backend.update_command_list(optimized_commands);
            m_render_backend.set_cursor();
        }
        texture_region load_image(const image_desc& image) override {
            return m_image_compactor.compact(image);
        }
    };
    std::unique_ptr<context> create_animgui_context(input_backend& input_backend, font_backend& font_backend,
                                                    render_backend& render_backend, emitter& emitter, animator& animator,
                                                    std::pmr::memory_resource* memory_manager) {
        return std::make_unique<context_impl>(input_backend, font_backend, render_backend, emitter, animator, memory_manager);
    }
}  // namespace animgui
