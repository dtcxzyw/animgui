// SPDX-License-Identifier: MIT

#pragma once
#include "emitter.hpp"
#include <optional>

namespace animgui {
    class input_backend;
    enum class key_code : uint32_t;
    struct style;
    using raw_callback = void (*)(void*, size_t);

    // TODO: focus to a widget
    class canvas {
    public:
        canvas() = default;
        virtual ~canvas() = default;
        canvas(const canvas& rhs) = delete;
        canvas(canvas&& rhs) = default;
        canvas& operator=(const canvas& rhs) = delete;
        canvas& operator=(canvas&& rhs) = default;

        [[nodiscard]] virtual vec2 calculate_bounds(const primitive& primitive) const = 0;
        [[nodiscard]] virtual identifier region_sub_uid() = 0;

        virtual std::pair<size_t, identifier> push_region(identifier uid,
                                                          const std::optional<bounds_aabb>& reserved_bounds = std::nullopt) = 0;
        virtual void pop_region(const std::optional<bounds_aabb>& new_bounds = std::nullopt) = 0;
        virtual std::pair<size_t, identifier> add_primitive(identifier uid, primitive primitive) = 0;
        [[nodiscard]] virtual vec2 reserved_size() const noexcept = 0;
        virtual span<operation> commands() noexcept = 0;

        virtual void* raw_storage(size_t hash, identifier uid) = 0;
        virtual void register_type(size_t hash, size_t size, size_t alignment, raw_callback ctor, raw_callback dtor) = 0;
        template <typename T>
        T& storage(const identifier uid) {
            const auto hash = typeid(T).hash_code();
            register_type(
                hash, sizeof(T), alignof(T),
                [](void* ptr, size_t size) { std::uninitialized_default_construct_n(static_cast<T*>(ptr), size); },
                [](void* ptr, size_t size) { std::destroy_n(static_cast<T*>(ptr), size); });
            return *static_cast<T*>(raw_storage(hash, uid));
        }

        [[nodiscard]] virtual const style& global_style() const noexcept = 0;
        [[nodiscard]] virtual std::pmr::memory_resource* memory_resource() const noexcept = 0;
        [[nodiscard]] virtual input_backend& input() const noexcept = 0;
        [[nodiscard]] virtual float delta_t() const noexcept = 0;

        // TODO: convex polygon/SDF bounds
        [[nodiscard]] virtual const bounds_aabb& region_bounds() const = 0;
        [[nodiscard]] virtual vec2 region_offset() const = 0;
        [[nodiscard]] virtual bool region_hovered() const = 0;
        [[nodiscard]] virtual bool hovered(const bounds_aabb& bounds) const = 0;
        [[nodiscard]] virtual bool region_request_focus(bool force = false) = 0;

        [[nodiscard]] virtual float step(identifier id, float dest) = 0;
    };
}  // namespace animgui
