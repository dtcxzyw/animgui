// SPDX-License-Identifier: MIT

#include <animgui/builtins/image_compactors.hpp>
#include <animgui/core/image_compactor.hpp>
#include <cassert>
#include <optional>

namespace animgui {
    static constexpr uint32_t image_pool_size = 1024;

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

        void create_new_column(const uint32_t start) {
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

    class image_compactor_impl final : public image_compactor {
        std::pmr::vector<compacted_image> m_images[3];
        render_backend& m_backend;
        std::pmr::memory_resource* m_memory_resource;

    public:
        explicit image_compactor_impl(render_backend& render_backend, std::pmr::memory_resource* memory_resource)
            : m_images{ std::pmr::vector<compacted_image>{ memory_resource },
                        std::pmr::vector<compacted_image>{ memory_resource },
                        std::pmr::vector<compacted_image>{ memory_resource } },
              m_backend{ render_backend }, m_memory_resource{ memory_resource } {}

        void reset() override {
            for(auto&& images : m_images)
                images.clear();
        }
        texture_region compact(const image_desc& image) override {
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

    ANIMGUI_API std::shared_ptr<image_compactor> create_builtin_image_compactor(render_backend& render_backend,
                                                                                std::pmr::memory_resource* memory_resource) {
        return std::make_shared<image_compactor_impl>(render_backend, memory_resource);
    }

}  // namespace animgui
