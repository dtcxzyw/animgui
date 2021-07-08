// SPDX-License-Identifier: MIT

#include <algorithm>
#include <animgui/builtins/command_optimizers.hpp>
#include <animgui/core/command_optimizer.hpp>
#include <functional>
#include <vector>

namespace animgui {
    class command_optimizer_noop final : public command_optimizer {
    public:
        [[nodiscard]] std::pmr::vector<command> optimize(uvec2, std::pmr::vector<command> src) const override {
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
        using command_queue = std::pmr::vector<command>;
        using command_pusher = std::function<void(command)>;

        void merge_tex(command_queue::iterator beg, const command_queue::iterator end, const command_pusher& push) const {
            if(beg == end)
                return;
            if(std::distance(beg, end) == 1) {
                push(std::move(*beg));
                return;
            }

            command current_command{ bounds_aabb::escaped(), std::nullopt, {} };
            for(auto last_size = std::numeric_limits<float>::infinity(); beg != end; ++beg) {
                if(const auto current_size = std::get<primitives>(beg->desc).point_line_size;
                   std::fabs(last_size - current_size) > 1e-3f) {
                    if(!current_command.bounds.is_escaped())
                        push(std::move(current_command));
                    current_command = std::move(*beg);
                    last_size = current_size;
                } else {
                    current_command.bounds.left = std::fmin(current_command.bounds.left, beg->bounds.left);
                    current_command.bounds.right = std::fmax(current_command.bounds.right, beg->bounds.right);
                    current_command.bounds.top = std::fmin(current_command.bounds.top, beg->bounds.top);
                    current_command.bounds.bottom = std::fmax(current_command.bounds.bottom, beg->bounds.bottom);
                    auto&& vertex_buffer = std::get<primitives>(current_command.desc).vertices;
                    const auto& source = std::get<primitives>(beg->desc).vertices;
                    vertex_buffer.insert(vertex_buffer.cend(), source.cbegin(), source.cend());
                }
            }
            push(std::move(current_command));
        }

        void merge_primitive(command_queue::iterator beg, const command_queue::iterator end, const command_pusher& push) const {
            if(beg == end)
                return;
            if(std::distance(beg, end) == 1 || beg->desc.index() == 0) {
                for(; beg != end; ++beg)
                    push(std::move(*beg));
                return;
            }

            std::sort(beg, end, [](const command& lhs, const command& rhs) {
                return std::get<primitives>(lhs.desc).tex < std::get<primitives>(rhs.desc).tex;
            });

            auto last = beg;
            for(auto last_tex = reinterpret_cast<void*>(std::numeric_limits<size_t>::max()); beg != end; ++beg) {
                if(const auto current_tex = std::get<primitives>(beg->desc).tex.get(); last_tex != current_tex) {
                    merge_tex(last, beg, push);
                    last = beg;
                    last_tex = current_tex;
                }
            }
            merge_tex(last, end, push);
        }

        void merge_clip(command_queue::iterator beg, const command_queue::iterator end, const command_pusher& push) const {
            if(beg == end)
                return;
            if(std::distance(beg, end) == 1) {
                push(std::move(*beg));
                return;
            }

            std::sort(beg, end, [](const command& lhs, const command& rhs) {
                if(lhs.desc.index() != rhs.desc.index())
                    return lhs.desc.index() < rhs.desc.index();
                if(lhs.desc.index() == 0)
                    return false;
                return std::get<primitives>(lhs.desc).type < std::get<primitives>(rhs.desc).type;
            });

            auto last = beg;
            for(uint32_t last_index = std::numeric_limits<uint32_t>::max(); beg != end; ++beg) {
                if(const auto current_index =
                       beg->desc.index() == 0 ? 0 : 1 + static_cast<uint32_t>(std::get<primitives>(beg->desc).type);
                   current_index != last_index) {
                    merge_primitive(last, beg, push);
                    last = beg;
                    last_index = current_index;
                }
            }
            merge_primitive(last, end, push);
        }

    public:
        [[nodiscard]] std::pmr::vector<command> optimize(uvec2 size, command_queue src) const override {
            const auto memory_resource = src.get_allocator().resource();

            std::pmr::vector<std::tuple<command, std::pmr::vector<size_t>, size_t>> stage1{ memory_resource };
            stage1.reserve(src.size());

            identifier clip_hash = {};
            auto last = src.begin();

            auto clipped = [](const bounds_aabb& clip, const bounds_aabb& bounds) {
                return bounds.left < clip.left || bounds.right > clip.right || bounds.top < clip.top ||
                    bounds.bottom > clip.bottom;
            };

            const command_pusher emit1 = [&](command cmd) {
                if(cmd.clip.has_value() && !clipped(cmd.clip.value(), cmd.bounds))
                    cmd.clip.reset();

                const auto cid = stage1.size();
                size_t prev_count = 0;

                for(auto&& [info, next, _] : stage1) {
                    if(intersect_bounds(info.bounds, cmd.bounds)) {
                        next.push_back(cid);
                        ++prev_count;
                    }
                }

                stage1.push_back({ std::move(cmd), std::pmr::vector<size_t>{ memory_resource }, prev_count });
            };

            auto magic = std::numeric_limits<size_t>::max();

            for(auto beg = src.begin(); beg != src.end(); ++beg) {
                if(const auto current_hash =
                       beg->clip.has_value() ? fnv1a_impl(&beg->clip.value(), sizeof(bounds_aabb)) : identifier{ --magic };
                   current_hash.id != clip_hash.id) {
                    merge_clip(last, beg, emit1);
                    last = beg;
                    clip_hash = current_hash;
                }
            }
            merge_clip(last, src.end(), emit1);

            std::pmr::vector<command> stage2{ memory_resource };
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
                if(std::get<command>(stage1[idx]).clip.has_value()) {
                    stage2.push_back(std::move(std::get<command>(stage1[idx])));
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

            const command_pusher emit3 = [&](command cmd) { stage2.push_back(std::move(cmd)); };

            while(!active_group.empty()) {
                std::pmr::vector<size_t> current_group{ memory_resource };
                current_group.swap(active_group);

                std::pmr::vector<command> group{ memory_resource };
                group.reserve(current_group.size());
                for(auto idx : current_group)
                    group.push_back(std::move(std::get<command>(stage1[idx])));

                merge_clip(group.begin(), group.end(), emit3);

                for(auto idx : current_group)
                    release(stage1[idx]);
            }

            return stage2;
        }
        [[nodiscard]] primitive_type supported_primitives() const noexcept override {
            return primitive_type::points | primitive_type::lines | primitive_type::triangles | primitive_type::quads;
        }
    };

    ANIMGUI_API std::shared_ptr<command_optimizer> create_builtin_command_optimizer() {
        return std::make_shared<command_optimizer_builtin>();
    }
}  // namespace animgui
