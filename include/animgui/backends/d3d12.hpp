// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <functional>
#include <memory>
struct ID3D12Device;
struct ID3D12GraphicsCommandList;

namespace animgui {
    class render_backend;
    using synchronized_executor = std::function<void(const std::function<void(ID3D12GraphicsCommandList*)>&)>;

    ANIMGUI_API std::shared_ptr<render_backend>
    create_d3d12_backend(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, uint32_t sample_count,
                         synchronized_executor synchronized_transferer, std::function<void(long)> error_checker);
}  // namespace animgui
