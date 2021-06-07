// SPDX-License-Identifier: MIT

#pragma once
#include "render_backend.hpp"

namespace animgui {
    class image_compactor {
    public:
        image_compactor() = default;
        virtual ~image_compactor() = default;
        image_compactor(const image_compactor& rhs) = delete;
        image_compactor(image_compactor&& rhs) = default;
        image_compactor& operator=(const image_compactor& rhs) = delete;
        image_compactor& operator=(image_compactor&& rhs) = default;
        virtual void reset() = 0;
        virtual texture_region compact(const image_desc& image) = 0;
    };
}  // namespace animgui
