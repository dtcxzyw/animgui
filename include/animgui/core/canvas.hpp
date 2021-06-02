// SPDX-License-Identifier: MIT

#pragma once
#include "emitter.hpp"

namespace animgui {
    struct style;

    class canvas {
    protected:
        using callback = void (*)(void*, size_t);
        virtual void* storage(size_t hash, uid uid) = 0;
        virtual void register_type(size_t hash, callback ctor, callback dtor);

    public:
        canvas() = default;
        virtual ~canvas() = default;
        canvas(const canvas& rhs) = delete;
        canvas(canvas&& rhs) = default;
        canvas& operator=(const canvas& rhs) = delete;
        canvas& operator=(canvas&& rhs) = default;

        virtual uid push_region(uid uid, const bounds& bounds) = 0;
        virtual void pop_region() = 0;
        virtual uid add_primitive(uid uid, const primitive& primitive) = 0;
        virtual style& style() noexcept = 0;
        template <typename T>
        T& storage(const uid uid) {
            const auto hash = typeid(T).hash_code();
            register_type(
                hash, [](void* ptr, size_t size) { std::uninitialized_default_construct_n(static_cast<T*>(ptr), size); },
                [](void* ptr, size_t size) { std::destroy_n(static_cast<T*>(ptr), size); });
            return storage(hash, uid);
        }

        // TODO: convex polygon bounds
        [[nodiscard]] virtual bounds widget_bounds() const = 0;
        [[nodiscard]] virtual bool widget_hovered() const = 0;
        [[nodiscard]] virtual bool widget_pressed() const = 0;
        [[nodiscard]] virtual bool hovered(const bounds& bounds) const = 0;
        [[nodiscard]] virtual bool pressed(const bounds& bounds) const = 0;

        [[nodiscard]] virtual vec2 reserved_size() = 0;
        virtual std::pmr::vector<operation>& commands() = 0;
        virtual void set_cursor(cursor cursor) = 0;
    };
}  // namespace animgui
