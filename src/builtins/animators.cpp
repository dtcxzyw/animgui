// SPDX-License-Identifier: MIT

#include <animgui/builtins/animators.hpp>
#include <animgui/core/animator.hpp>

namespace animgui {
    class dummy_animator final : public animator {
    public:
        [[nodiscard]] step_function step(float) const override {
            return [](const float dest, void*) { return dest; };
        }
        [[nodiscard]] std::tuple<size_t, size_t, size_t> state_storage() const noexcept override {
            return { 0, 0, 0 };
        }
    };

    std::shared_ptr<animator> create_dummy_animator() {
        return std::make_shared<dummy_animator>();
    }

    class linear_animator final : public animator {
        float m_speed;

        struct storage final {
            float current;
        };

    public:
        explicit linear_animator(const float speed) : m_speed{ speed } {}
        [[nodiscard]] step_function step(float delta_t) const override {
            return [dx = delta_t * m_speed](const float dest, void* data) {
                auto&& state = static_cast<storage*>(data);
                if(fabsf(state->current - dest) < dx)
                    state->current = dest;
                else
                    state->current += copysignf(dx, dest - state->current);
                return state->current;
            };
        }
        [[nodiscard]] std::tuple<size_t, size_t, size_t> state_storage() const noexcept override {
            return { typeid(storage).hash_code(), sizeof(storage), alignof(storage) };
        }
    };

    std::shared_ptr<animator> create_linear_animator(float speed) {
        return std::make_shared<linear_animator>(speed);
    }

    class physical_animator final : public animator {
        float m_speed;

        struct storage final {
            float current;
        };

    public:
        explicit physical_animator(const float speed) : m_speed{ speed } {}
        [[nodiscard]] step_function step(float delta_t) const override {
            return [factor = std::expf(-m_speed * delta_t)](const float dest, void* data) {
                auto&& state = static_cast<storage*>(data);
                state->current = dest + (state->current - dest) * factor;
                return state->current;
            };
        }
        [[nodiscard]] std::tuple<size_t, size_t, size_t> state_storage() const noexcept override {
            return { typeid(storage).hash_code(), sizeof(storage), alignof(storage) };
        }
    };

    std::shared_ptr<animator> create_physical_animator(const float speed) {
        return std::make_shared<physical_animator>(speed);
    }
}  // namespace animgui
