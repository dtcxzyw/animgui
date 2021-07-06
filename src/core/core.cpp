// SPDX-License-Identifier: MIT

#include <animgui/builtins/styles.hpp>
#include <animgui/core/animator.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/command_optimizer.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/font_backend.hpp>
#include <animgui/core/image_compactor.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/style.hpp>
#include <cstring>
#include <list>
#include <optional>
#include <random>
#include <set>
#include <stack>
#include <cmath>

namespace animgui {
    class state_manager final {
        std::pmr::unordered_map<identifier, std::pair<size_t, size_t>, identifier_hasher> m_state_location;
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
        void* storage(const size_t hash, const identifier uid) {
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
    struct region_info final {
        size_t push_command_idx;
        identifier uid;
        std::minstd_rand random_engine;
        bounds_aabb absolute_bounds;
        vec2 offset;

        region_info() = delete;
    };

    class canvas_impl final : public canvas {
        context& m_context;
        vec2 m_size;
        input_backend& m_input_backend;
        step_function m_step_function;
        emitter& m_emitter;
        state_manager& m_state_manager;
        std::pmr::memory_resource* m_memory_resource;
        input_mode m_input_mode;
        std::pmr::vector<operation> m_commands;
        std::pmr::deque<region_info> m_region_stack;
        size_t m_animation_state_hash;
        std::pmr::vector<std::pair<identifier, vec2>> m_focusable_region;

    public:
        canvas_impl(context& context, const vec2 size, input_backend& input, animator& animator, const float delta_t,
                    emitter& emitter, state_manager& state_manager, std::pmr::memory_resource* memory_resource)
            : m_context{ context }, m_size{ size }, m_input_backend{ input }, m_step_function{ animator.step(delta_t) },
              m_emitter{ emitter }, m_state_manager{ state_manager }, m_memory_resource{ memory_resource },
              m_input_mode{ m_input_backend.get_input_mode() }, m_commands{ m_memory_resource },
              m_region_stack{ m_memory_resource }, m_animation_state_hash{ 0 }, m_focusable_region{ memory_resource } {
            const auto [hash, state_size, alignment] = animator.state_storage();
            m_animation_state_hash = hash;
            m_state_manager.register_type(
                hash, state_size, alignment, [](void*, size_t) {}, [](void*, size_t) {});
            m_region_stack.push_back({ std::numeric_limits<size_t>::max(),
                                       identifier{ 0 },
                                       std::minstd_rand{},  // NOLINT(cert-msc51-cpp)
                                       bounds_aabb{ 0.0f, size.x, 0.0f, size.y },
                                       { 0.0f, 0.0f } });
        }
        [[nodiscard]] const style& global_style() const noexcept override {
            return m_context.global_style();
        }
        [[nodiscard]] void* raw_storage(const size_t hash, const identifier uid) override {
            return m_state_manager.storage(hash, uid);
        }
        span<operation> commands() noexcept override {
            return { m_commands.data(), m_commands.data() + m_commands.size() };
        }
        [[nodiscard]] vec2 reserved_size() const noexcept override {
            for(auto iter = m_region_stack.rbegin(); iter != m_region_stack.rend(); ++iter) {
                const auto idx = iter->push_command_idx;
                if(idx == std::numeric_limits<size_t>::max())
                    break;
                if(const auto size = std::get<op_push_region>(m_commands[idx]).bounds.size(); size.x > 0.0f && size.y > 0.0f)
                    return size;
            }
            return m_size;
        }
        void register_type(const size_t hash, const size_t size, const size_t alignment, const raw_callback ctor,
                           const raw_callback dtor) override {
            m_state_manager.register_type(hash, size, alignment, ctor, dtor);
        }
        void pop_region(const std::optional<bounds_aabb>& bounds) override {
            m_commands.push_back(op_pop_region{});
            const auto& info = m_region_stack.back();
            const auto idx = info.push_command_idx;
            const auto uid = info.uid;
            m_region_stack.pop_back();
            auto&& current_bounds = std::get<op_push_region>(m_commands[idx]).bounds;
            if(bounds.has_value())
                current_bounds = bounds.value();
            storage<bounds_aabb>(mix(uid, "last_bounds"_id)) = current_bounds;
        }
        [[nodiscard]] identifier current_region_uid() const {
            return m_region_stack.back().uid;
        }
        std::pair<size_t, identifier> push_region(const identifier uid, const std::optional<bounds_aabb>& bounds) override {
            const auto idx = m_commands.size();
            m_commands.push_back(op_push_region{ bounds.value_or(bounds_aabb{ 0.0f, 0.0f, 0.0f, 0.0f }) });
            const auto mixed = mix(current_region_uid(), uid);
            auto last_bounds = storage<bounds_aabb>(mix(mixed, "last_bounds"_id));
            const auto& parent = m_region_stack.back();
            const vec2 offset{ last_bounds.left, last_bounds.top };
            clip_bounds(last_bounds, parent.offset, parent.absolute_bounds);
            m_region_stack.push_back(
                { idx, mixed, std::minstd_rand{ static_cast<unsigned>(mixed.id) }, last_bounds, parent.offset + offset });
            return { idx, mixed };
        }
        [[nodiscard]] const bounds_aabb& region_bounds() const override {
            return m_region_stack.back().absolute_bounds;
        }
        [[nodiscard]] bool region_hovered() const override {
            return hovered(region_bounds());
        }
        [[nodiscard]] bool hovered(const bounds_aabb& bounds) const override {
            const auto [x, y] = m_input_backend.get_cursor_pos();
            return bounds.left <= x && x < bounds.right && bounds.top <= y && y < bounds.bottom;
        }
        std::pair<size_t, identifier> add_primitive(const identifier uid, primitive primitive) override {
            const auto idx = m_commands.size();
            m_commands.push_back(std::move(primitive));
            return { idx, mix(current_region_uid(), uid) };
        }
        [[nodiscard]] std::pmr::memory_resource* memory_resource() const noexcept override {
            return m_memory_resource;
        }
        [[nodiscard]] float step(const identifier id, const float dest) override {
            return m_step_function(dest, m_animation_state_hash ? raw_storage(m_animation_state_hash, id) : nullptr);
        }
        [[nodiscard]] vec2 calculate_bounds(const primitive& primitive) const override {
            return m_emitter.calculate_bounds(primitive, m_context.global_style());
        }
        identifier region_sub_uid() override {
            return mix(current_region_uid(), identifier{ m_region_stack.back().random_engine() });
        }
        [[nodiscard]] input_backend& input() const noexcept override {
            return m_input_backend;
        }
        bool region_request_focus(const bool force) override {
            if(m_input_mode != input_mode::game_pad)
                return false;
            const auto bounds = m_region_stack.back().absolute_bounds;
            const vec2 center = { (bounds.left + bounds.right) / 2.0f, (bounds.top + bounds.bottom) / 2.0f };
            const auto current = current_region_uid();
            m_focusable_region.push_back({ current, center });
            auto& last_focus = storage<identifier>("glabal_focus"_id);
            if(force) {
                last_focus = current;
                return true;
            }
            return last_focus == current;
        }
        void finish() {
            // TODO: mode switching
            auto& last_focus = storage<identifier>("glabal_focus"_id);
            if(m_input_mode != input_mode::game_pad || m_focusable_region.empty()) {
                last_focus = identifier{ 0 };
                return;
            }
            vec2 focus{ 0.0f, 0.0f };
            bool lost_focus = true;
            for(auto& [id, pos] : m_focusable_region) {
                if(last_focus == id) {
                    focus = pos;
                    lost_focus = false;
                    break;
                }
            }
            if(lost_focus) {
                const auto init =
                    std::min_element(m_focusable_region.cbegin(), m_focusable_region.cend(), [](auto lhs, auto rhs) {
                        return std::fabs(lhs.second.y - rhs.second.y) < 0.1f ? lhs.second.x < rhs.second.x :
                                                                               lhs.second.y < rhs.second.y;
                    });
                focus = init->second;
                last_focus = init->first;
            }

            auto dir = m_input_backend.action_direction_pulse_repeated(true);

            if(dir.x * dir.x + dir.y * dir.y < 0.25f) {
                return;
            }

            const auto norm = std::hypot(dir.x, dir.y);
            dir.x /= norm;
            dir.y /= norm;

            auto min_dist = std::numeric_limits<float>::max();

            for(auto& [id, pos] : m_focusable_region) {
                if(id == last_focus)
                    continue;

                const auto diff = pos - focus;
                const auto dot = diff.x * dir.x + diff.y * dir.y;
                if(dot < 0.01f)
                    continue;

                if(const auto diameter = (diff.x * diff.x + diff.y * diff.y) / std::pow(dot, 1.4f); diameter < min_dist) {
                    min_dist = diameter;
                    last_focus = id;
                }
            }
        }
        [[nodiscard]] vec2 region_offset() const override {
            return m_region_stack.back().offset;
        }
    };

    // TODO: improve performance
    class codepoint_locator final {
        std::pmr::unordered_map<font*, std::pmr::unordered_map<uint32_t, texture_region>> m_lut;
        image_compactor& m_image_compactor;

        std::pmr::unordered_map<uint32_t, texture_region>& locate(font& font_ref) {
            const auto iter = m_lut.find(&font_ref);
            if(iter == m_lut.cend())
                return m_lut
                    .emplace(&font_ref, std::pmr::unordered_map<uint32_t, texture_region>{ m_lut.get_allocator().resource() })
                    .first->second;
            return iter->second;
        }

    public:
        explicit codepoint_locator(image_compactor& image_compactor, std::pmr::memory_resource* memory_resource)
            : m_lut{ memory_resource }, m_image_compactor{ image_compactor } {}
        void reset() {
            m_lut.clear();
        }
        texture_region locate(font& font_ref, const glyph_id glyph) {
            auto&& lut = locate(font_ref);
            const auto iter = lut.find(glyph.idx);
            if(iter == lut.cend()) {
                return lut
                    .emplace(
                        glyph.idx,
                        font_ref.render_to_bitmap(
                            glyph, [&](const image_desc& desc) { return m_image_compactor.compact(desc, font_ref.max_scale()); }))
                    .first->second;
            }
            return iter->second;
        }
    };

    class command_fallback_translator final {
        primitive_type m_supported_primitive;
        primitive_type m_fallback_primitive;
        void (*m_quad_emitter)(std::pmr::vector<vertex>&, const vertex&, const vertex&, const vertex&, const vertex&);

        // CCW
        static void emit_triangle(std::pmr::vector<vertex>& vertices, const vertex& p1, const vertex& p2, const vertex& p3) {
            vertices.push_back(p1);
            vertices.push_back(p2);
            vertices.push_back(p3);
        }
        static void emit_quad_triangle_strip(std::pmr::vector<vertex>& vertices, const vertex& p1, const vertex& p2,
                                             const vertex& p3, const vertex& p4) {
            vertices.push_back(p1);
            vertices.push_back(p2);
            vertices.push_back(p4);
            vertices.push_back(p3);
        }
        static void emit_quad_triangles(std::pmr::vector<vertex>& vertices, const vertex& p1, const vertex& p2, const vertex& p3,
                                        const vertex& p4) {
            vertices.push_back(p1);
            vertices.push_back(p2);
            vertices.push_back(p3);
            vertices.push_back(p1);
            vertices.push_back(p3);
            vertices.push_back(p4);
        }
        void emit_line(std::pmr::vector<vertex>& vertices, const vertex& p1, const vertex& p2, const float width) const {
            auto dx = p1.pos.x - p2.pos.x, dy = p1.pos.y - p2.pos.y;
            const auto dist = std::hypotf(dx, dy);
            if(dist < 0.1f)
                return;
            const auto scale = 0.5f * width / dist;
            dx *= scale;
            dy *= -scale;
            std::swap(dx, dy);
            vertex p11 = p1, p12 = p1, p21 = p2, p22 = p2;
            p11.pos.x += dx;
            p11.pos.y += dy;
            p12.pos.x -= dx;
            p12.pos.y -= dy;
            p21.pos.x += dx;
            p21.pos.y += dy;
            p22.pos.x -= dx;
            p22.pos.y -= dy;
            m_quad_emitter(vertices, p11, p21, p22, p12);
        }

        void fallback_lines_adj(const std::pmr::vector<vertex>& input, std::pmr::vector<vertex>& output, const float line_width,
                                const bool make_loop) const {
            for(size_t i = 1; i < input.size(); ++i)
                emit_line(output, input[i - 1], input[i], line_width);
            if(make_loop)
                emit_line(output, input.back(), input.front(), line_width);
        }
        void fallback_lines(const std::pmr::vector<vertex>& input, std::pmr::vector<vertex>& output,
                            const float line_width) const {
            for(size_t i = 1; i < input.size(); i += 2)
                emit_line(output, input[i - 1], input[i], line_width);
        }
        void fallback_points(const std::pmr::vector<vertex>& input, std::pmr::vector<vertex>& output,
                             const float point_size) const {
            const auto offset = point_size * 0.5f;
            for(auto&& p0 : input) {
                vertex p1 = p0, p2 = p0, p3 = p0, p4 = p0;
                p1.pos.x -= offset;
                p1.pos.y -= offset;
                p2.pos.x -= offset;
                p2.pos.y += offset;
                p3.pos.x += offset;
                p3.pos.y += offset;
                p4.pos.x += offset;
                p4.pos.y -= offset;
                m_quad_emitter(output, p1, p2, p3, p4);
            }
        }
        void fallback_quads(const std::pmr::vector<vertex>& input, std::pmr::vector<vertex>& output) const {
            for(size_t base = 0; base < input.size(); base += 4)
                m_quad_emitter(output, input[base], input[base + 1], input[base + 2], input[base + 3]);
        }

        static void fallback_triangle_strip(const std::pmr::vector<vertex>& input, std::pmr::vector<vertex>& output) {
            for(size_t i = 2; i < input.size(); ++i) {
                if(i & 1)
                    emit_triangle(output, input[i], input[i - 1], input[i - 2]);
                else
                    emit_triangle(output, input[i], input[i - 2], input[i - 1]);
            }
        }

        static void fallback_triangle_fan(const std::pmr::vector<vertex>& input, std::pmr::vector<vertex>& output) {
            for(size_t i = 2; i < input.size(); ++i)
                emit_triangle(output, input[0], input[i - 1], input[i]);
        }

    public:
        explicit command_fallback_translator(const primitive_type supported_primitive)
            : m_supported_primitive{ supported_primitive },
              m_fallback_primitive{ support_primitive(supported_primitive, primitive_type::triangle_strip) ?
                                        primitive_type::triangle_strip :
                                        primitive_type::triangles },
              m_quad_emitter{ support_primitive(supported_primitive, primitive_type::triangle_strip) ? emit_quad_triangle_strip :
                                                                                                       emit_quad_triangles } {
            if(!support_primitive(m_supported_primitive, primitive_type::triangles))
                throw std::logic_error{ "Unsupported render backend" };
        }

        void transform(std::pmr::vector<command>& command_list) const {
            for(auto& [_, desc] : command_list)
                if(desc.index() == 1) {
                    // ReSharper disable once CppTooWideScope
                    auto&& [type, vertices, _, point_line_size] = std::get<primitives>(desc);
                    if(!support_primitive(m_supported_primitive, type)) {
                        std::pmr::vector<vertex> output{ vertices.get_allocator().resource() };
                        output.reserve(vertices.size() * 3);
                        // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement CppIncompleteSwitchStatement
                        switch(type) {  // NOLINT(clang-diagnostic-switch)
                            case primitive_type::points:
                                fallback_points(vertices, output, point_line_size);
                                break;
                            case primitive_type::lines:
                                fallback_lines(vertices, output, point_line_size);
                                break;
                            case primitive_type::line_strip:
                                fallback_lines_adj(vertices, output, point_line_size, false);
                                break;
                            case primitive_type::line_loop:
                                fallback_lines_adj(vertices, output, point_line_size, true);
                                break;
                            case primitive_type::triangle_fan:
                                fallback_triangle_fan(vertices, output);
                                break;
                            case primitive_type::triangle_strip:
                                fallback_triangle_strip(vertices, output);
                                break;
                            case primitive_type::quads:
                                fallback_quads(vertices, output);
                                break;
                        }
                        type = m_fallback_primitive;
                        vertices = std::move(output);
                    }
                }
        }
    };

    class context_impl final : public context {
        input_backend& m_input_backend;
        render_backend& m_render_backend;
        font_backend& m_font_backend;
        emitter& m_emitter;
        animator& m_animator;
        command_optimizer& m_command_optimizer;
        image_compactor& m_image_compactor;

        state_manager m_state_manager;
        codepoint_locator m_codepoint_locator;
        command_fallback_translator m_command_fallback_translator;
        std::pmr::memory_resource* m_memory_resource;
        style m_style;

    public:
        context_impl(input_backend& input_backend, render_backend& render_backend, font_backend& font_backend, emitter& emitter,
                     animator& animator, command_optimizer& command_optimizer, image_compactor& image_compactor,
                     std::pmr::memory_resource* memory_resource)
            : m_input_backend{ input_backend }, m_render_backend{ render_backend }, m_font_backend{ font_backend },
              m_emitter{ emitter }, m_animator{ animator }, m_command_optimizer{ command_optimizer },
              m_image_compactor{ image_compactor }, m_state_manager{ memory_resource }, m_codepoint_locator{ image_compactor,
                                                                                                             memory_resource },
              m_command_fallback_translator{ render_backend.supported_primitives() },
              m_memory_resource{ memory_resource }, m_style{ nullptr, {}, {}, {}, {}, {}, {}, {}, 0.0f } {
            set_classic_style(*this);
        }
        void reset_cache() override {
            m_state_manager.reset();
            m_codepoint_locator.reset();
            m_image_compactor.reset();
        }
        style& global_style() noexcept override {
            return m_style;
        }
        void new_frame(const uint32_t width, const uint32_t height, const float delta_t,
                       const std::function<void(canvas&)>& render_function) override {
            std::pmr::monotonic_buffer_resource arena{ 1 << 15, m_memory_resource };

            canvas_impl canvas_root{ *this,           vec2{ static_cast<float>(width), static_cast<float>(height) },
                                     m_input_backend, m_animator,
                                     delta_t,         m_emitter,
                                     m_state_manager, &arena };
            render_function(canvas_root);
            canvas_root.finish();

            auto commands = m_emitter.transform(canvas_root.reserved_size(), canvas_root.commands(), m_style,
                                                [&](font& font_ref, const glyph_id glyph) -> texture_region {
                                                    return m_codepoint_locator.locate(font_ref, glyph);
                                                });
            auto optimized_commands = m_command_optimizer.optimize(std::move(commands));
            m_command_fallback_translator.transform(optimized_commands);
            m_render_backend.update_command_list(uvec2{ width, height }, std::move(optimized_commands));
        }
        texture_region load_image(const image_desc& image, const float max_scale) override {
            return m_image_compactor.compact(image, max_scale);
        }
        [[nodiscard]] std::shared_ptr<font> load_font(const std::pmr::string& name, const float height) const override {
            return m_font_backend.load_font(name, height);
        }
    };
    ANIMGUI_API std::unique_ptr<context> create_animgui_context(input_backend& input_backend, render_backend& render_backend,
                                                                font_backend& font_backend, emitter& emitter, animator& animator,
                                                                command_optimizer& command_optimizer,
                                                                image_compactor& image_compactor,
                                                                std::pmr::memory_resource* memory_manager) {
        return std::make_unique<context_impl>(input_backend, render_backend, font_backend, emitter, animator, command_optimizer,
                                              image_compactor, memory_manager);
    }
    texture_region texture_region::sub_region(const bounds_aabb& bounds) const {
        const auto w = region.right - region.left, h = region.bottom - region.top;
        return { tex,
                 { region.left + w * bounds.left, region.left + w * bounds.right, region.top + h * bounds.top,
                   region.top + h * bounds.bottom } };
    }
}  // namespace animgui
