// SPDX-License-Identifier: MIT

#pragma once
#include <memory>

namespace animgui {
    class emitter;

    std::unique_ptr<emitter> create_builtin_emitter();
}
