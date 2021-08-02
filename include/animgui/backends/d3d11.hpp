// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <functional>
#include <memory>
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace animgui {
    class render_backend;
    ANIMGUI_API std::shared_ptr<render_backend> create_d3d11_backend(ID3D11Device* device, ID3D11DeviceContext* device_context,
                                                                     std::function<void(long)> error_checker);
}  // namespace animgui
