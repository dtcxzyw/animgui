// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <vulkan/vulkan.hpp>

namespace animgui {
    class render_backend;
    using synchronized_executor = std::function<void(const std::function<void(vk::CommandBuffer&)>&)>;

    ANIMGUI_API std::shared_ptr<render_backend>
    create_vulkan_backend(vk::PhysicalDevice& physical_device, vk::Device& device, vk::RenderPass& render_pass,
                          vk::CommandBuffer& command_buffer, vk::SampleCountFlagBits sample_count, float sample_shading,
                          synchronized_executor synchronized_transfer, std::function<void(vk::Result)> error_report);
}  // namespace animgui
