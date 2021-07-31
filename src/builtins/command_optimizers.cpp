// SPDX-License-Identifier: MIT

#include <algorithm>
#include <animgui/builtins/command_optimizers.hpp>
#include <animgui/core/command_optimizer.hpp>
#include <functional>
#include <vector>

namespace animgui {
    class command_optimizer_noop final : public command_optimizer {
    public:
        [[nodiscard]] command_queue optimize(uvec2, command_queue src) const override {
            // noop
            return src;
        }
        [[nodiscard]] primitive_type supported_primitives() const noexcept override {
            return primitive_type::points | primitive_type::lines | primitive_type::line_strip | primitive_type::line_loop |
                primitive_type::triangles | primitive_type::triangle_fan | primitive_type::triangle_strip | primitive_type::quads;
        }
    };

    ANIMGUI_API std::shared_ptr<command_optimizer> create_noop_command_optimizer() {
        return std::make_shared<command_optimizer_noop>();
    }

    class command_optimizer_builtin final : public command_optimizer {
        using command_queue_sub = std::pmr::vector<std::pair<command, std::pmr::vector<std::pair<uint32_t, uint32_t>>>>;
        using command_pusher = std::function<void(std::pair<command, std::pmr::vector<std::pair<uint32_t, uint32_t>>>)>;

        void merge_tex(command_queue_sub::iterator beg, const command_queue_sub::iterator end, const command_pusher& push) const {
            if(beg == end)
                return;
            if(std::distance(beg, end) == 1) {
                push(std::move(*beg));
                return;
            }

            command_queue_sub::value_type current_command{ { bounds_aabb::escaped(), std::nullopt, {} }, {} };
            for(auto last_size = std::numeric_limits<float>::infinity(); beg != end; ++beg) {
                if(const auto current_size = std::get<primitives>(beg->first.desc).point_line_size;
                   std::fabs(last_size - current_size) > 1e-3f) {
                    if(!current_command.first.bounds.is_escaped())
                        push(std::move(current_command));
                    current_command = std::move(*beg);
                    last_size = current_size;
                } else {
                    current_command.first.bounds.left = std::fmin(current_command.first.bounds.left, beg->first.bounds.left);
                    current_command.first.bounds.right = std::fmax(current_command.first.bounds.right, beg->first.bounds.right);
                    current_command.first.bounds.top = std::fmin(current_command.first.bounds.top, beg->first.bounds.top);
                    current_command.first.bounds.bottom =
                        std::fmax(current_command.first.bounds.bottom, beg->first.bounds.bottom);
                    current_command.second.insert(current_command.second.end(), beg->second.begin(), beg->second.end());
                }
            }
            push(std::move(current_command));
        }

        void merge_primitive(command_queue_sub::iterator beg, const command_queue_sub::iterator end,
                             const command_pusher& push) const {
            if(beg == end)
                return;
            if(std::distance(beg, end) == 1 || beg->first.desc.index() == 0) {
                for(; beg != end; ++beg)
                    push(std::move(*beg));
                return;
            }

            std::sort(beg, end, [](const command_queue_sub::value_type& lhs, const command_queue_sub::value_type& rhs) {
                return std::get<primitives>(lhs.first.desc).tex < std::get<primitives>(rhs.first.desc).tex;
            });

            auto last = beg;
            for(auto last_tex = reinterpret_cast<void*>(std::numeric_limits<size_t>::max()); beg != end; ++beg) {
                if(const auto current_tex = std::get<primitives>(beg->first.desc).tex.get(); last_tex != current_tex) {
                    merge_tex(last, beg, push);
                    last = beg;
                    last_tex = current_tex;
                }
            }
            merge_tex(last, end, push);
        }

        void merge_clip(command_queue_sub::iterator beg, const command_queue_sub::iterator end,
                        const command_pusher& push) const {
            if(beg == end)
                return;
            if(std::distance(beg, end) == 1) {
                push(std::move(*beg));
                return;
            }

            std::sort(beg, end, [](const command_queue_sub::value_type& lhs, const command_queue_sub::value_type& rhs) {
                if(lhs.first.desc.index() != rhs.first.desc.index())
                    return lhs.first.desc.index() < rhs.first.desc.index();
                if(lhs.first.desc.index() == 0)
                    return false;
                return std::get<primitives>(lhs.first.desc).type < std::get<primitives>(rhs.first.desc).type;
            });

            auto last = beg;
            for(uint32_t last_index = std::numeric_limits<uint32_t>::max(); beg != end; ++beg) {
                if(const auto current_index =
                       beg->first.desc.index() == 0 ? 0 : 1 + static_cast<uint32_t>(std::get<primitives>(beg->first.desc).type);
                   current_index != last_index) {
                    merge_primitive(last, beg, push);
                    last = beg;
                    last_index = current_index;
                }
            }
            merge_primitive(last, end, push);
        }

    public:
        [[nodiscard]] command_queue optimize(uvec2 size, command_queue src) const override {
            const auto memory_resource = src.vertices.get_allocator().resource();

            std::pmr::vector<std::tuple<command_queue_sub::value_type, std::pmr::vector<size_t>, size_t>> stage1{
                memory_resource
            };
            stage1.reserve(src.commands.size());

            auto clipped = [](const bounds_aabb& clip, const bounds_aabb& bounds) {
                return bounds.left < clip.left || bounds.right > clip.right || bounds.top < clip.top ||
                    bounds.bottom > clip.bottom;
            };

            const command_pusher emit1 = [&](command_queue_sub::value_type cmd) {
                if(cmd.first.clip.has_value() && !clipped(cmd.first.clip.value(), cmd.first.bounds))
                    cmd.first.clip.reset();

                const auto cid = stage1.size();
                size_t prev_count = 0;

                for(auto&& [info, next, _] : stage1) {
                    if(intersect_bounds(info.first.bounds, cmd.first.bounds)) {
                        next.push_back(cid);
                        ++prev_count;
                    }
                }

                stage1.push_back({ std::move(cmd), std::pmr::vector<size_t>{ memory_resource }, prev_count });
            };

            auto magic = std::numeric_limits<size_t>::max();

            command_queue_sub src_sub{ memory_resource };

            {
                src_sub.reserve(src.commands.size());
                uint32_t vertices_offset = 0;
                for(auto& command : src.commands) {
                    if(auto desc = std::get_if<primitives>(&command.desc)) {
                        src_sub.push_back(std::make_pair(std::move(command),
                                                         std::pmr::vector<std::pair<uint32_t, uint32_t>>{
                                                             { { vertices_offset, desc->vertices_count } }, memory_resource }));
                        vertices_offset += desc->vertices_count;
                    } else
                        src_sub.push_back(std::make_pair(std::move(command), std::pmr::vector<std::pair<uint32_t, uint32_t>>{}));
                }
            }

            identifier clip_hash = {};
            auto last = src_sub.begin();
            for(auto beg = src_sub.begin(); beg != src_sub.end(); ++beg) {
                if(const auto current_hash = beg->first.clip.has_value() ?
                       fnv1a_impl(&beg->first.clip.value(), sizeof(bounds_aabb)) :
                       identifier{ --magic };
                   current_hash.id != clip_hash.id) {
                    merge_clip(last, beg, emit1);
                    last = beg;
                    clip_hash = current_hash;
                }
            }
            merge_clip(last, src_sub.end(), emit1);

            std::pmr::vector<command_queue_sub::value_type> stage2{ memory_resource };
            stage2.reserve(stage1.size());

            std::pmr::vector<size_t> active_group{ memory_resource };

            std::function<void(size_t)> emit2;

            auto release = [&](const auto& node) {
                for(auto idx : std::get<std::pmr::vector<size_t>>(node)) {
                    if(--std::get<size_t>(stage1[idx]) == 0) {
                        emit2(idx);
                    }
                }
            };

            emit2 = [&](const size_t idx) {
                if(std::get<command_queue_sub::value_type>(stage1[idx]).first.clip.has_value()) {
                    stage2.push_back(std::move(std::get<command_queue_sub::value_type>(stage1[idx])));
                    release(stage1[idx]);
                } else
                    active_group.push_back(idx);
            };

            std::pmr::vector<size_t> start{ memory_resource };
            for(size_t idx = 0; idx < stage1.size(); ++idx) {
                if(std::get<size_t>(stage1[idx]) == 0)
                    start.push_back(idx);
            }

            for(auto idx : start)
                emit2(idx);

            const command_pusher emit3 = [&](command_queue_sub::value_type cmd) { stage2.push_back(std::move(cmd)); };

            while(!active_group.empty()) {
                std::pmr::vector<size_t> current_group{ memory_resource };
                current_group.swap(active_group);

                std::pmr::vector<command_queue_sub::value_type> group{ memory_resource };
                group.reserve(current_group.size());
                for(auto idx : current_group)
                    group.push_back(std::move(std::get<command_queue_sub::value_type>(stage1[idx])));

                merge_clip(group.begin(), group.end(), emit3);

                for(auto idx : current_group)
                    release(stage1[idx]);
            }

            std::pmr::vector<vertex> sorted_vertices{ memory_resource };
            sorted_vertices.reserve(src.vertices.size());
            std::pmr::vector<command> commands{ memory_resource };
            commands.reserve(stage2.size());

            for(auto&& cmd : stage2) {
                if(auto desc = std::get_if<primitives>(&cmd.first.desc)) {
                    uint32_t vertices_count = 0;
                    for(auto&& range : cmd.second) {
                        sorted_vertices.insert(sorted_vertices.end(), src.vertices.begin() + range.first,
                                               src.vertices.begin() + range.first + range.second);
                        vertices_count += range.second;
                    }
                    desc->vertices_count = vertices_count;
                }
                commands.push_back(std::move(cmd.first));
            }

            return { std::move(sorted_vertices), std::move(commands) };
        }
        [[nodiscard]] primitive_type supported_primitives() const noexcept override {
            return primitive_type::points | primitive_type::lines | primitive_type::triangles | primitive_type::quads;
        }
    };

    ANIMGUI_API std::shared_ptr<command_optimizer> create_builtin_command_optimizer() {
        return std::make_shared<command_optimizer_builtin>();
    }
}  // namespace animgui
