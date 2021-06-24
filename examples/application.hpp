// SPDX-License-Identifier: MIT

#pragma once
#include <memory>

namespace animgui {
    class canvas;
    class context;

    class application {
    public:
        application() = default;
        virtual ~application() = default;
        application(const application& rhs) = delete;
        application(application&& rhs) = delete;
        application& operator=(const application& rhs) = delete;
        application& operator=(application&& rhs) = delete;

        virtual void render(canvas& canvas_root) = 0;
    };

    std::shared_ptr<application> create_demo_application(context& context);
}  // namespace animgui
