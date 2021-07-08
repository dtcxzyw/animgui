// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace animgui {
    struct pipeline_statistics final {
        uint32_t smooth_fps;

        // unit: us
        uint32_t frame_time;
        uint32_t input_time;
        uint32_t draw_time;
        uint32_t emit_time;
        uint32_t fallback_time;
        uint32_t optimize_time;
        uint32_t render_time;

        // float input_latency;
        // float render_latency;

        uint32_t generated_operation;
        uint32_t emitted_draw_call;
        uint32_t transformed_draw_call;
        uint32_t optimized_draw_call;
    };
}  // namespace animgui
