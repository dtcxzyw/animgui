// SPDX-License-Identifier: MIT

#include <animgui/core/animator.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/font_backend.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/style.hpp>
#include <optional>
#include <stack>

namespace animgui {
    class state_manager final {
        std::pmr::unordered_map<uid, std::pair<size_t, size_t>, uid_hasher> m_state_location;
        class state_buffer final {
            size_t m_state_size, m_alignment;
            raw_callback m_ctor, m_dtor;
            std::pmr::memory_resource* m_memory_resource;
            void* m_buffer;
            size_t m_buffer_size, m_allocated_size;

        public:
            state_buffer(std::pmr::memory_resource* memory_resource, const size_t size, const size_t alignment,
                         const raw_callback ctor, const raw_callback dtor)
                : m_state_size{ size }, m_alignment{ alignment }, m_ctor{ ctor }, m_dtor{ dtor },
                  m_memory_resource{ memory_resource }, m_buffer{ nullptr }, m_buffer_size{ 0 }, m_allocated_size{ 0 } {}
            ~state_buffer() {
                if(m_buffer) {
                    m_dtor(m_buffer, m_buffer_size);
                    m_memory_resource->deallocate(m_buffer, m_buffer_size * m_state_size, m_alignment);
                }
            }
            state_buffer(const state_buffer& rhs) = delete;
            state_buffer(state_buffer&& rhs) noexcept
                : m_state_size{ rhs.m_state_size }, m_alignment{ rhs.m_alignment }, m_ctor{ rhs.m_ctor }, m_dtor{ rhs.m_dtor },
                  m_memory_resource{ rhs.m_memory_resource }, m_buffer{ rhs.m_buffer }, m_buffer_size{ rhs.m_buffer_size },
                  m_allocated_size{ rhs.m_allocated_size } {
                rhs.m_buffer = nullptr;
                rhs.m_allocated_size = rhs.m_buffer_size = 0;
            }
            state_buffer& operator=(const state_buffer& rhs) = delete;
            state_buffer& operator=(state_buffer&& rhs) noexcept {
                state_buffer tmp{ std::move(rhs) };
                swap(tmp);
                return *this;
            }
            void swap(state_buffer& rhs) noexcept {
                std::swap(m_state_size, rhs.m_state_size);
                std::swap(m_alignment, rhs.m_alignment);
                std::swap(m_ctor, rhs.m_ctor);
                std::swap(m_memory_resource, rhs.m_memory_resource);
                std::swap(m_buffer, rhs.m_buffer);
                std::swap(m_buffer_size, rhs.m_buffer_size);
                std::swap(m_allocated_size, rhs.m_allocated_size);
            }

            size_t new_storage() {
                if(m_buffer_size == m_allocated_size) {
                    const auto new_size = std::max(static_cast<size_t>(128), m_allocated_size * 2);
                    const auto new_ptr = m_memory_resource->allocate(new_size * m_state_size, m_alignment);
                    memcpy(new_ptr, m_buffer, m_buffer_size * m_state_size);
                    m_memory_resource->deallocate(m_buffer, m_allocated_size * m_state_size, m_alignment);
                    m_allocated_size = new_size;
                    m_buffer = new_ptr;
                }
                const auto idx = m_buffer_size;
                m_ctor(locate(idx), 1);
                ++m_buffer_size;
                return idx;
            }
            [[nodiscard]] void* locate(const size_t idx) const noexcept {
                return static_cast<std::byte*>(m_buffer) + idx * m_state_size;
            }
        };
        std::pmr::unordered_map<size_t, state_buffer> m_state_storage;

    public:
        explicit state_manager(std::pmr::memory_resource* memory_resource)
            : m_state_location{ memory_resource }, m_state_storage{ memory_resource } {}
        void reset() {
            m_state_location.clear();
            m_state_storage.clear();
        }
        void* storage(const size_t hash, const uid uid) {
            const auto iter = m_state_location.find(uid);
            if(iter == m_state_location.cend()) {
                auto&& buffer = m_state_storage.find(hash)->second;
                size_t idx = buffer.new_storage();
                m_state_location.emplace(uid, std::make_pair(hash, idx));
                return buffer.locate(idx);
            }
            if(iter->second.first != hash)
                throw std::logic_error("hash collision");
            return m_state_storage.find(hash)->second.locate(iter->second.second);
        }
        void register_type(const size_t hash, size_t size, size_t alignment, raw_callback ctor, raw_callback dtor) {
            if(!m_state_storage.count(hash))
                m_state_storage.emplace(
                    std::piecewise_construct, std::forward_as_tuple(hash),
                    std::forward_as_tuple(m_state_storage.get_allocator().resource(), size, alignment, ctor, dtor));
        }
    };
    class canvas_impl final : public canvas {
        context& m_context;
        vec2 m_size;
        input_backend& m_input_backend;
        step_function m_step_function;
        emitter& m_emitter;
        state_manager& m_state_manager;
        std::pmr::memory_resource* m_memory_resource;
        cursor m_cursor;
        std::pmr::vector<operation> m_commands;
        std::stack<std::pair<size_t, uid>, std::pmr::deque<std::pair<size_t, uid>>> m_region_stack;
        size_t m_animation_state_hash;

    public:
        canvas_impl(context& context, vec2 size, input_backend& input_backend, animator& animator, const float delta_t,
                    emitter& emitter, state_manager& state_manager, std::pmr::memory_resource* memory_resource)
            : m_context{ context }, m_size{ size }, m_input_backend{ input_backend },
              m_step_function{ animator.step(delta_t) }, m_emitter{ emitter }, m_state_manager{ state_manager },
              m_memory_resource{ memory_resource }, m_cursor{ cursor::arrow }, m_commands{ m_memory_resource },
              m_region_stack{ m_memory_resource }, m_animation_state_hash{ 0 } {
            const auto [hash, state_size, alignment] = animator.state_storage();
            m_animation_state_hash = hash;
            m_state_manager.register_type(
                hash, state_size, alignment, [](void*, size_t) {}, [](void*, size_t) {});
        }
        animgui::style& style() noexcept override {
            return m_context.style();
        }
        [[nodiscard]] void* raw_storage(const size_t hash, const uid uid) override {
            return m_state_manager.storage(hash, uid);
        }
        span<operation> commands() override {
            return { m_commands.data(), m_commands.data() + m_commands.size() };
        }
        [[nodiscard]] vec2 reserved_size() override {
            return m_size;
        }
        void register_type(const size_t hash, const size_t size, const size_t alignment, const raw_callback ctor,
                           const raw_callback dtor) override {
            m_state_manager.register_type(hash, size, alignment, ctor, dtor);
        }
        void pop_region() override {
            m_commands.push_back(animgui::pop_region{});
            const auto [idx, uid] = m_region_stack.top();
            m_region_stack.pop();
            storage<bounds>(mix(uid, "last_bounds"_id)) = std::get<animgui::push_region>(m_commands[idx]).bounds;
        }
        [[nodiscard]] uid current_region_uid() const {
            return m_region_stack.empty() ? animgui::uid{ 0 } : m_region_stack.top().second;
        }
        uid push_region(const uid uid, const bounds& bounds) override {
            const auto idx = m_commands.size();
            m_commands.push_back(animgui::push_region{ bounds });
            const auto mixed = mix(current_region_uid(), uid);
            m_region_stack.emplace(idx, mixed);
            return mixed;
        }
        [[nodiscard]] bool pressed(const key_code key, const bounds& bounds) const override {
            return m_input_backend.get_key(key) && hovered(bounds);
        }
        [[nodiscard]] bool region_pressed(const key_code key) const override {
            return pressed(key, region_bounds());
        }
        [[nodiscard]] bounds region_bounds() const override {
            if(m_region_stack.empty())
                return { 0.0f, 0.0f, m_size.x, m_size.y };
            const auto uid = m_region_stack.top().second;
            return const_cast<canvas_impl*>(this)->storage<bounds>(mix(uid, "last_bounds"_id));
        }
        [[nodiscard]] bool region_hovered() const override {
            return hovered(region_bounds());
        }
        [[nodiscard]] bool hovered(const bounds& bounds) const override {
            // TODO: window pos to canvas pos
            const auto [x, y] = m_input_backend.get_cursor_pos();
            return bounds.left <= x && x < bounds.right && bounds.top <= y && y < bounds.bottom;
        }
        void set_cursor(const cursor cursor) override {
            if(cursor != cursor::arrow)
                m_cursor = cursor;
        }
        uid add_primitive(const uid uid, primitive primitive) override {
            m_commands.push_back(std::move(primitive));
            return mix(current_region_uid(), uid);
        }
        [[nodiscard]] std::pmr::memory_resource* memory_resource() const noexcept override {
            return m_memory_resource;
        }
        [[nodiscard]] float step(const uid id, const float dest) override {
            return m_step_function(dest, m_animation_state_hash ? raw_storage(m_animation_state_hash, id) : nullptr);
        }
        [[nodiscard]] cursor cursor() const noexcept {
            return m_cursor;
        }
        [[nodiscard]] vec2 calculate_bounds(const primitive& primitive) const override {
            return m_emitter.calculate_bounds(primitive);
        }
    };
    class command_optimizer final {
    public:
        [[nodiscard]] std::pmr::vector<command> optimize(const std::pmr::vector<command>& src) const {
            // noop
            // TODO: merge draw calls
            return src;
        }
    };

    class compacted_image final {
        std::shared_ptr<texture> m_texture;
        static constexpr uint32_t margin = 1;

    public:
        explicit compacted_image(std::shared_ptr<texture> texture) : m_texture{ std::move(texture) } {}
        [[nodiscard]] std::shared_ptr<texture> texture() const {
            return m_texture;
        }
        std::optional<bounds> allocate(const image_desc& image) {
            // notice: no padding
            return std::nullopt;
        }
    };
    class image_compactor final {
        std::pmr::vector<compacted_image> m_images[3];
        render_backend& m_backend;
        static constexpr uint32_t image_pool_size = 1024;

    public:
        explicit image_compactor(render_backend& render_backend, std::pmr::memory_resource* memory_manager)
            : m_images{ std::pmr::vector<compacted_image>{ memory_manager }, std::pmr::vector<compacted_image>{ memory_manager },
                        std::pmr::vector<compacted_image>{ memory_manager } },
              m_backend{ render_backend } {}
        ~image_compactor() = default;
        image_compactor(const image_compactor& rhs) = delete;
        image_compactor(image_compactor&& rhs) = default;
        image_compactor& operator=(const image_compactor& rhs) = delete;
        image_compactor& operator=(image_compactor&& rhs) = delete;

        void reset() {
            for(auto&& images : m_images)
                images.clear();
        }
        texture_region compact(const image_desc& image) {
            if(std::max(image.width, image.height) >= image_pool_size) {
                auto tex = m_backend.create_texture(image.width, image.height, image.channels);
                tex->update_texture(0, 0, image);
                return { tex, bounds{ 0.0f, 1.0f, 0.0f, 1.0f } };
            }
            auto& images = m_images[static_cast<uint32_t>(image.channels)];
            for(auto&& pool : images) {
                if(auto bounds = pool.allocate(image)) {
                    return { pool.texture(), bounds.value() };
                }
            }
            images.emplace_back(m_backend.create_texture(image_pool_size, image_pool_size, image.channels));
            auto&& pool = images.back();
            return { pool.texture(), pool.allocate(image).value() };
        }
    };
    // TODO: improve performance
    class codepoint_locator final {
        std::pmr::unordered_map<font*, std::pmr::unordered_map<uint32_t, texture_region>> m_lut;
        image_compactor& m_image_compactor;

        std::pmr::unordered_map<uint32_t, texture_region>& locate(font& font) {
            const auto iter = m_lut.find(&font);
            if(iter == m_lut.cend())
                return m_lut.emplace(&font, std::pmr::unordered_map<uint32_t, texture_region>{ m_lut.get_allocator().resource() })
                    .first->second;
            return iter->second;
        }

    public:
        explicit codepoint_locator(image_compactor& image_compactor, std::pmr::memory_resource* memory_resource)
            : m_lut{ memory_resource }, m_image_compactor{ image_compactor } {}
        void reset() {
            m_lut.clear();
        }
        texture_region locate(font& font, const uint32_t codepoint) {
            auto lut = locate(font);
            const auto iter = lut.find(codepoint);
            if(iter == lut.cend()) {
                const auto height = static_cast<uint32_t>(ceil(font.height()));
                const auto width = static_cast<uint32_t>(ceil(font.calculate_width(codepoint)));
                std::pmr::vector<std::byte> buffer{ width * height, m_lut.get_allocator().resource() };
                const image_desc image{ width, height, channel::alpha, buffer.data() };
                font.render_to_bitmap(codepoint, image);
                return lut.emplace(codepoint, m_image_compactor.compact(image)).first->second;
            }
            return iter->second;
        }
    };
    class context_impl final : public context {
        input_backend& m_input_backend;
        render_backend& m_render_backend;
        emitter& m_emitter;
        animator& m_animator;

        state_manager m_state_manager;
        image_compactor m_image_compactor;
        codepoint_locator m_codepoint_locator;
        command_optimizer m_command_optimizer;
        std::pmr::memory_resource* m_memory_resource;
        animgui::style m_style;

    public:
        context_impl(input_backend& input_backend, render_backend& render_backend, emitter& emitter, animator& animator,
                     std::pmr::memory_resource* memory_resource)
            : m_input_backend{ input_backend }, m_render_backend{ render_backend }, m_emitter{ emitter }, m_animator{ animator },
              m_state_manager{ memory_resource }, m_image_compactor{ render_backend, memory_resource },
              m_codepoint_locator{ m_image_compactor, memory_resource }, m_memory_resource{ memory_resource }, m_style{
                  nullptr, '?', nullptr, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },
                  nullptr
              } {}
        void reset_cache() override {
            m_state_manager.reset();
            m_codepoint_locator.reset();
            m_image_compactor.reset();
        }
        animgui::style& style() noexcept override {
            return m_style;
        }
        void new_frame(const uint32_t width, const uint32_t height, const float delta_t,
                       const std::function<void(canvas& canvas)>& render_function) override {
            std::pmr::monotonic_buffer_resource arena{ 1 << 15, m_memory_resource };
            canvas_impl canvas{ *this,           vec2{ static_cast<float>(width), static_cast<float>(height) },
                                m_input_backend, m_animator,
                                delta_t,         m_emitter,
                                m_state_manager, &arena };
            render_function(canvas);
            const auto commands = m_emitter.transform(canvas.reserved_size(), canvas.commands(), m_style,
                                                      [&](font& font, const uint32_t codepoint) -> texture_region {
                                                          return m_codepoint_locator.locate(font, codepoint);
                                                      });
            const auto optimized_commands = m_command_optimizer.optimize(commands);
            m_render_backend.update_command_list(optimized_commands);
            m_render_backend.set_cursor(canvas.cursor());
        }
        texture_region load_image(const image_desc& image) override {
            return m_image_compactor.compact(image);
        }
    };
    std::unique_ptr<context> create_animgui_context(input_backend& input_backend, render_backend& render_backend,
                                                    emitter& emitter, animator& animator,
                                                    std::pmr::memory_resource* memory_manager) {
        return std::make_unique<context_impl>(input_backend, render_backend, emitter, animator, memory_manager);
    }
    texture_region texture_region::sub_region(const bounds& bounds) const {
        const auto w = region.right - region.left, h = region.bottom - region.top;
        return { texture,
                 { region.left + w * bounds.left, region.left + w * bounds.right, region.top + h * bounds.top,
                   region.top + h * bounds.bottom } };
    }

}  // namespace animgui
