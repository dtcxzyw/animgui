// SPDX-License-Identifier: MIT

#pragma once
#include "emitter.hpp"

namespace animgui {
    enum class key_code : uint32_t;
    struct style;
    using raw_callback = void (*)(void*, size_t);

    class canvas {
    public:
        canvas() = default;
        virtual ~canvas() = default;
        canvas(const canvas& rhs) = delete;
        canvas(canvas&& rhs) = default;
        canvas& operator=(const canvas& rhs) = delete;
        canvas& operator=(canvas&& rhs) = default;

        virtual std::pair<size_t, uid> push_region(uid uid, const bounds& bounds) = 0;
        virtual void pop_region() = 0;
        virtual std::pair<size_t, uid> add_primitive(uid uid, primitive primitive) = 0;
        virtual style& style() noexcept = 0;
        virtual void* raw_storage(size_t hash, uid uid) = 0;
        virtual void register_type(size_t hash, size_t size, size_t alignment, raw_callback ctor, raw_callback dtor) = 0;
        template <typename T>
        T& storage(const uid uid) {
            const auto hash = typeid(T).hash_code();
            register_type(
                hash, sizeof(T), alignof(T),
                [](void* ptr, size_t size) { std::uninitialized_default_construct_n(static_cast<T*>(ptr), size); },
                [](void* ptr, size_t size) { std::destroy_n(static_cast<T*>(ptr), size); });
            return *static_cast<T*>(raw_storage(hash, uid));
        }

        // TODO: convex polygon/SDF bounds
        [[nodiscard]] virtual bounds region_bounds() const = 0;
        [[nodiscard]] virtual bool region_hovered() const = 0;
        [[nodiscard]] virtual bool region_pressed(key_code key) const = 0;
        [[nodiscard]] virtual bool hovered(const bounds& bounds) const = 0;
        [[nodiscard]] virtual bool pressed(key_code key, const bounds& bounds) const = 0;

        [[nodiscard]] virtual vec2 reserved_size() = 0;
        virtual span<operation> commands() = 0;
        virtual void set_cursor(cursor cursor) = 0;
        [[nodiscard]] virtual float step(uid id, float dest) = 0;
        [[nodiscard]] virtual std::pmr::memory_resource* memory_resource() const noexcept = 0;
        [[nodiscard]] virtual vec2 calculate_bounds(const primitive& primitive) const = 0;
        [[nodiscard]] virtual uid region_sub_uid() = 0;
    };
}  // namespace animgui
