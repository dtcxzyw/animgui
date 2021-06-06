// SPDX-License-Identifier: MIT

#include <animgui/builtins/styles.hpp>
#include <animgui/core/animator.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/font_backend.hpp>
#include <animgui/core/input_backend.hpp>
#include <animgui/core/style.hpp>
#include <cassert>
#include <list>
#include <optional>
#include <random>
#include <set>
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
        stack<std::tuple<size_t, uid, std::minstd_rand, bounds>> m_region_stack;
        size_t m_animation_state_hash;

    public:
        canvas_impl(context& context, const vec2 size, input_backend& input_backend, animator& animator, const float delta_t,
                    emitter& emitter, state_manager& state_manager, std::pmr::memory_resource* memory_resource)
            : m_context{ context }, m_size{ size }, m_input_backend{ input_backend },
              m_step_function{ animator.step(delta_t) }, m_emitter{ emitter }, m_state_manager{ state_manager },
              m_memory_resource{ memory_resource }, m_cursor{ cursor::arrow }, m_commands{ m_memory_resource },
              m_region_stack{ m_memory_resource }, m_animation_state_hash{ 0 } {
            const auto [hash, state_size, alignment] = animator.state_storage();
            m_animation_state_hash = hash;
            m_state_manager.register_type(
                hash, state_size, alignment, [](void*, size_t) {}, [](void*, size_t) {});
            m_region_stack.push(std::make_tuple(std::numeric_limits<size_t>::max(), uid{ 0 }, std::minstd_rand{},
                                                bounds{ 0.0f, size.x, 0.0f, size.y }));  // NOLINT(cert-msc51-cpp)
        }
        animgui::style& style() noexcept override {
            return m_context.style();
        }
        [[nodiscard]] void* raw_storage(const size_t hash, const uid uid) override {
            return m_state_manager.storage(hash, uid);
        }
        span<operation> commands() noexcept override {
            return { m_commands.data(), m_commands.data() + m_commands.size() };
        }
        [[nodiscard]] vec2 reserved_size() const noexcept override {
            return m_size;
        }
        void register_type(const size_t hash, const size_t size, const size_t alignment, const raw_callback ctor,
                           const raw_callback dtor) override {
            m_state_manager.register_type(hash, size, alignment, ctor, dtor);
        }
        void pop_region(const std::optional<bounds>& bounds) override {
            m_commands.push_back(animgui::pop_region{});
            const auto idx = std::get<size_t>(m_region_stack.top());
            const auto uid = std::get<animgui::uid>(m_region_stack.top());
            m_region_stack.pop();
            auto&& current_bounds = std::get<animgui::push_region>(m_commands[idx]).bounds;
            if(bounds.has_value())
                current_bounds = bounds.value();
            storage<animgui::bounds>(mix(uid, "last_bounds"_id)) = current_bounds;
        }
        [[nodiscard]] uid current_region_uid() const {
            return std::get<uid>(m_region_stack.top());
        }
        std::pair<size_t, uid> push_region(const uid uid, const std::optional<bounds>& bounds) override {
            const auto idx = m_commands.size();
            m_commands.push_back(animgui::push_region{ bounds.value_or(animgui::bounds{ 0.0f, 0.0f, 0.0f, 0.0f }) });
            const auto mixed = mix(current_region_uid(), uid);
            auto last_bounds = storage<animgui::bounds>(mix(mixed, "last_bounds"_id));
            clip_bounds(last_bounds, std::get<animgui::bounds>(m_region_stack.top()));
            m_region_stack.emplace(idx, mixed, std::minstd_rand{ static_cast<unsigned>(mixed.id) }, last_bounds);
            return { idx, mixed };
        }
        [[nodiscard]] bool pressed(const key_code key, const bounds& bounds) const override {
            return m_input_backend.get_key(key) && hovered(bounds);
        }
        [[nodiscard]] bool region_pressed(const key_code key) const override {
            return pressed(key, region_bounds());
        }
        [[nodiscard]] const bounds& region_bounds() const override {
            return std::get<bounds>(m_region_stack.top());
        }
        [[nodiscard]] bool region_hovered() const override {
            return hovered(region_bounds());
        }
        [[nodiscard]] bool hovered(const bounds& bounds) const override {
            // TODO: window pos to canvas pos
            const auto [x, y] = m_input_backend.get_cursor_pos();
            return bounds.left <= x && x < bounds.right && bounds.top <= y && y < bounds.bottom;
        }
        void set_cursor(const cursor cursor) noexcept override {
            if(cursor != cursor::arrow)
                m_cursor = cursor;
        }
        std::pair<size_t, uid> add_primitive(const uid uid, primitive primitive) override {
            const auto idx = m_commands.size();
            m_commands.push_back(std::move(primitive));
            return { idx, mix(current_region_uid(), uid) };
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
            return m_emitter.calculate_bounds(primitive, m_context.style());
        }
        uid region_sub_uid() override {
            return mix(current_region_uid(), uid{ std::get<std::minstd_rand>(m_region_stack.top())() });
        }
    };

    class command_optimizer final {
    public:
        [[nodiscard]] std::pmr::vector<command> optimize(std::pmr::vector<command> src) const {
            // noop
            // TODO: merge draw calls
            return src;
        }
    };

    static constexpr uint32_t image_pool_size = 1026;

    class compacted_image final {
        static constexpr uint32_t margin = 1;

        std::shared_ptr<texture> m_texture;
        std::pmr::memory_resource* m_memory_resource;

        struct tree_node final {
            int id;
            uvec2 size;
            uvec2 pos;
            std::pmr::vector<int> children;
            int parent;
            explicit tree_node(std::pmr::memory_resource* memory_resource)
                : id{ -1 }, size{}, pos{}, children{ memory_resource }, parent{ -1 } {}
        };

        struct root_info final {
            uint32_t right{ 0 };
            std::pmr::vector<tree_node> nodes;
        };

        uint32_t m_tex_w{ image_pool_size }, m_tex_h{ image_pool_size };
        std::pmr::vector<root_info> m_columns;

        void create_new_column(uint32_t start) {
            root_info column;
            column.right = m_tex_w;

            tree_node root(m_memory_resource);
            root.id = static_cast<int>(column.nodes.size());
            root.size.x = 0;
            root.size.y = m_tex_h;
            root.pos.x = start;
            root.pos.y = 0;

            column.nodes.push_back(root);
            if(!m_columns.empty())
                m_columns.back().right = start;
            m_columns.push_back(column);
        }

        [[nodiscard]] bounds update_texture(uvec2 offset, const image_desc& image) const {
            offset.x += margin;
            offset.y += margin;
            m_texture->update_texture(offset, image);
            constexpr float norm = image_pool_size;
            return { static_cast<float>(offset.x) / norm, static_cast<float>(offset.x + image.size.x) / norm,
                     static_cast<float>(offset.y) / norm, static_cast<float>(offset.y + image.size.y) / norm };
        }

    public:
        explicit compacted_image(std::shared_ptr<texture> texture, std::pmr::memory_resource* memory_resource)
            : m_texture{ std::move(texture) }, m_memory_resource{ memory_resource } {
            create_new_column(0);
        }
        [[nodiscard]] std::shared_ptr<texture> texture() const {
            return m_texture;
        }
        std::optional<bounds> allocate(const image_desc& image) {
            // Create a new node.
            tree_node new_node(m_memory_resource);
            new_node.size.x = image.size.x + margin * 2;
            new_node.size.y = image.size.y + margin * 2;
            // The image should be smaller than the size of texture.
            assert(new_node.size.x <= m_tex_w && new_node.size.y <= m_tex_h);
            // Find a place in existing columns.
            for(auto&& column : m_columns) {
                for(auto&& parent : column.nodes) {
                    if(parent.children.empty()) {
                        if(parent.size.y >= new_node.size.y && parent.pos.x + parent.size.x + new_node.size.x <= column.right) {
                            new_node.parent = parent.id;
                            new_node.pos.x = parent.pos.x + parent.size.x;
                            new_node.pos.y = parent.pos.y;
                            new_node.id = static_cast<int>(column.nodes.size());
                            parent.children.push_back(new_node.id);
                            column.nodes.push_back(new_node);
                            return update_texture(new_node.pos, image);
                        }
                    } else {
                        if(tree_node& side = column.nodes[parent.children.back()];
                           side.pos.y + side.size.y + new_node.size.y <= parent.pos.y + parent.size.y &&
                           parent.pos.x + parent.size.x + new_node.size.y <= column.right) {
                            new_node.parent = parent.id;
                            new_node.pos.x = parent.pos.x + parent.size.x;
                            new_node.pos.y = side.pos.y + side.size.y;
                            new_node.id = static_cast<int>(column.nodes.size());
                            parent.children.push_back(new_node.id);
                            column.nodes.push_back(new_node);
                            return update_texture(new_node.pos, image);
                        }
                    }
                }
            }
            // Create a new column.
            uint32_t right = 0;
            auto&& column = m_columns.back();
            for(auto&& node : column.nodes)
                right = std::max(right, node.pos.x + node.size.x);
            if(m_tex_w - right >= new_node.size.x) {
                create_new_column(right);
                auto&& new_column = m_columns.back();
                new_node.parent = 0;
                new_node.pos.x = right;
                new_node.pos.y = 0;
                new_node.id = 1;
                new_column.nodes[0].children.push_back(1);
                new_column.nodes.push_back(new_node);
                return update_texture(new_node.pos, image);
            }
            // Allocation failed.
            return std::nullopt;
        }
    };

    class image_compactor final {
        std::pmr::vector<compacted_image> m_images[3];
        render_backend& m_backend;
        std::pmr::memory_resource* m_memory_resource;

    public:
        explicit image_compactor(render_backend& render_backend, std::pmr::memory_resource* memory_resource)
            : m_images{ std::pmr::vector<compacted_image>{ memory_resource },
                        std::pmr::vector<compacted_image>{ memory_resource },
                        std::pmr::vector<compacted_image>{ memory_resource } },
              m_backend{ render_backend }, m_memory_resource{ memory_resource } {}
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
            if(std::max(image.size.x, image.size.y) >= image_pool_size) {
                auto tex = m_backend.create_texture(image.size, image.channels);
                tex->update_texture(uvec2{ 0, 0 }, image);
                return { std::move(tex), bounds{ 0.0f, 1.0f, 0.0f, 1.0f } };
            }
            auto& images = m_images[static_cast<uint32_t>(image.channels)];
            for(auto&& pool : images) {
                if(auto bounds = pool.allocate(image)) {
                    return { pool.texture(), bounds.value() };
                }
            }
            images.emplace_back(m_backend.create_texture({ image_pool_size, image_pool_size }, image.channels),
                                m_memory_resource);
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
        texture_region locate(font& font, const glyph glyph) {
            auto&& lut = locate(font);
            const auto iter = lut.find(glyph.idx);
            if(iter == lut.cend()) {
                return lut
                    .emplace(
                        glyph.idx,
                        font.render_to_bitmap(glyph, [&](const image_desc& desc) { return m_image_compactor.compact(desc); }))
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
                    auto&& [type, vertices, _, point_line_size] = std::get<animgui::primitives>(desc);
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
        emitter& m_emitter;
        animator& m_animator;

        state_manager m_state_manager;
        image_compactor m_image_compactor;
        codepoint_locator m_codepoint_locator;
        command_optimizer m_command_optimizer;
        command_fallback_translator m_command_fallback_translator;
        std::pmr::memory_resource* m_memory_resource;
        animgui::style m_style;

    public:
        context_impl(input_backend& input_backend, render_backend& render_backend, emitter& emitter, animator& animator,
                     std::pmr::memory_resource* memory_resource)
            : m_input_backend{ input_backend }, m_render_backend{ render_backend }, m_emitter{ emitter }, m_animator{ animator },
              m_state_manager{ memory_resource }, m_image_compactor{ render_backend, memory_resource },
              m_codepoint_locator{ m_image_compactor, memory_resource },
              m_command_fallback_translator{ render_backend.supported_primitives() },
              m_memory_resource{ memory_resource }, m_style{ nullptr, {}, {}, {}, {}, {}, {}, {}, 0.0f } {
            set_classic_style(*this);
        }
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
            auto commands = m_emitter.transform(
                canvas.reserved_size(), canvas.commands(), m_style,
                [&](font& font, const glyph glyph) -> texture_region { return m_codepoint_locator.locate(font, glyph); });
            auto optimized_commands = m_command_optimizer.optimize(std::move(commands));
            m_command_fallback_translator.transform(optimized_commands);
            // TODO: optimize again?
            m_render_backend.update_command_list(std::move(optimized_commands));
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
