// SPDX-License-Identifier: MIT

#include "../application.hpp"
#include <animgui/backends/vulkan.hpp>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <animgui/backends/glfw3.hpp>
#include <animgui/backends/stbfont.hpp>
#include <animgui/builtins/animators.hpp>
#include <animgui/builtins/command_optimizers.hpp>
#include <animgui/builtins/emitters.hpp>
#include <animgui/builtins/image_compactors.hpp>
#include <animgui/builtins/layouts.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/input_backend.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

[[noreturn]] void fail(const std::string& str) {
    std::cout << str << std::endl;
    std::_Exit(EXIT_FAILURE);
}

void check_vulkan_result(const vk::Result res) {
    if(res != vk::Result::eSuccess) {
        fail("Vulkan Error");
    }
}

int main() {
    glfwSetErrorCallback([](const int id, const char* error) { fail("[GLFW:" + std::to_string(id) + "] " + error); });
    if(!glfwInit())
        fail("Failed to initialize glfw");

    if(!glfwVulkanSupported())
        fail("Vulkan support is unavailable");

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    const int width = 1024, height = 768;
    GLFWwindow* const window = glfwCreateWindow(width, height, "Animgui demo (vulkan_glfw3)", nullptr, nullptr);
    int screen_w, screen_h;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), nullptr, nullptr, &screen_w, &screen_h);
    glfwSetWindowPos(window, (screen_w - width) / 2, (screen_h - height) / 2);

    try {
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();  // TODO: vulkan memory allocator

        const vk::ApplicationInfo application_info{ "animgui_demo", VK_MAKE_VERSION(1, 0, 0), "animgui", VK_MAKE_VERSION(1, 0, 0),
                                                    VK_API_VERSION_1_2 };

        uint32_t extensions_count;
        auto extensions_array = glfwGetRequiredInstanceExtensions(&extensions_count);

        // TODO: dynamic loader
        // TODO: debug callback
        const std::initializer_list<const char*> layers = {
#ifdef ANIMGUI_DEBUG
            "VK_LAYER_KHRONOS_validation"  //, "VK_LAYER_LUNARG_api_dump", "VK_LAYER_KHRONOS_synchronization2"
#endif
        };

        const auto instance = vk::createInstanceUnique(vk::InstanceCreateInfo{
            {}, &application_info, static_cast<uint32_t>(layers.size()), layers.begin(), extensions_count, extensions_array });
        vk::PhysicalDevice physical_device{};
        uint32_t queue_family = 0;
        auto sample_count = vk::SampleCountFlagBits::e1;
        vk::PhysicalDeviceMemoryProperties memory_prop;
        auto sample_shading = -1.0f;

        for(auto item : instance->enumeratePhysicalDevices()) {
            uint32_t suitable_queue_family = std::numeric_limits<uint32_t>::max();
            {
                const auto queue_family_properties = item.getQueueFamilyProperties();
                constexpr auto mask = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
                for(uint32_t idx = 0; idx < queue_family_properties.size(); ++idx) {
                    if((queue_family_properties[idx].queueFlags & mask) == mask) {
                        suitable_queue_family = idx;
                        break;
                    }
                }
            }
            if(suitable_queue_family != std::numeric_limits<uint32_t>::max() &&
               glfwGetPhysicalDevicePresentationSupport(instance.get(), item, suitable_queue_family)) {
                physical_device = item;

                if(const auto supported_multiple_sampling_flags =
                       physical_device.getProperties().limits.framebufferColorSampleCounts;
                   supported_multiple_sampling_flags & vk::SampleCountFlagBits::e64)
                    sample_count = vk::SampleCountFlagBits::e64;
                else if(supported_multiple_sampling_flags & vk::SampleCountFlagBits::e32)
                    sample_count = vk::SampleCountFlagBits::e32;
                else if(supported_multiple_sampling_flags & vk::SampleCountFlagBits::e16)
                    sample_count = vk::SampleCountFlagBits::e16;
                else if(supported_multiple_sampling_flags & vk::SampleCountFlagBits::e8)
                    sample_count = vk::SampleCountFlagBits::e8;
                else if(supported_multiple_sampling_flags & vk::SampleCountFlagBits::e4)
                    sample_count = vk::SampleCountFlagBits::e4;
                else if(supported_multiple_sampling_flags & vk::SampleCountFlagBits::e2)
                    sample_count = vk::SampleCountFlagBits::e2;

                if(physical_device.getFeatures().sampleRateShading)
                    sample_shading = 0.2f;

                queue_family = suitable_queue_family;
                memory_prop = physical_device.getMemoryProperties();
                break;
            }
        }

        if(physical_device == static_cast<vk::PhysicalDevice>(nullptr))
            fail("No physical device for presentation");

        std::cout << "Physical Device: " << physical_device.getProperties().deviceName.data() << std::endl;

        const auto queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_create_info{ {}, queue_family, 1, &queue_priority };

        const std::initializer_list<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        vk::PhysicalDeviceFeatures enabled_features{};
        if(sample_shading > 0.0f)
            enabled_features.sampleRateShading = true;
        auto device = physical_device.createDeviceUnique(vk::DeviceCreateInfo{ {},
                                                                               1,
                                                                               &queue_create_info,
                                                                               0,
                                                                               nullptr,
                                                                               static_cast<uint32_t>(extensions.size()),
                                                                               extensions.begin(),
                                                                               &enabled_features });
        auto queue = device->getQueue(queue_family, 0);
        const auto command_pool = device->createCommandPoolUnique(
            vk::CommandPoolCreateInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family });

        vk::UniqueSurfaceKHR surface;
        {
            VkSurfaceKHR surface_handle;
            glfwCreateWindowSurface(instance.get(), window, nullptr, &surface_handle);
            surface = vk::UniqueSurfaceKHR{ surface_handle, { instance.get() } };
        }

        if(!physical_device.getSurfaceSupportKHR(queue_family, surface.get()))
            fail("Unsupported surface");
        vk::SurfaceFormatKHR selected_format{};
        {
            const auto formats = physical_device.getSurfaceFormatsKHR(surface.get());
            selected_format = formats.front();
            for(auto fmt : formats) {
                if(fmt.format == vk::Format::eB8G8R8A8Unorm && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                    selected_format = fmt;
                    break;
                }
            }
        }
        vk::PresentModeKHR present_mode{};
        if(const auto present_modes = physical_device.getSurfacePresentModesKHR(surface.get());
           std::find(present_modes.cbegin(), present_modes.cend(), vk::PresentModeKHR::eImmediate) != present_modes.cend())
            present_mode = vk::PresentModeKHR::eImmediate;
        else if(std::find(present_modes.cbegin(), present_modes.cend(), vk::PresentModeKHR::eMailbox) != present_modes.cend())
            present_mode = vk::PresentModeKHR::eMailbox;
        else if(std::find(present_modes.cbegin(), present_modes.cend(), vk::PresentModeKHR::eFifo) != present_modes.cend())
            present_mode = vk::PresentModeKHR::eFifo;
        else
            present_mode = present_modes.front();

        vk::UniqueSwapchainKHR swap_chain;
        vk::UniqueRenderPass render_pass;
        std::vector<vk::UniqueCommandBuffer> command_buffers;
        vk::CommandBuffer current_command_buffer;
        std::pmr::vector<vk::UniqueFramebuffer> frame_buffers{ memory_resource };
        std::pmr::vector<vk::UniqueImageView> swap_chain_image_views{ memory_resource };
        constexpr uint32_t max_frames_in_flight = 2;
        std::pmr::vector<vk::UniqueSemaphore> render_finished_semaphores{ max_frames_in_flight, memory_resource };
        std::pmr::vector<vk::UniqueSemaphore> image_available_semaphores{ max_frames_in_flight, memory_resource };
        std::pmr::vector<std::pair<vk::UniqueFence, bool>> in_flight_fences{ max_frames_in_flight, memory_resource };
        std::pmr::vector<vk::Fence> images_in_flight{ memory_resource };
        vk::UniqueImage multiple_sampled_image;
        vk::UniqueDeviceMemory multiple_sampled_image_memory;
        vk::UniqueImageView multiple_sampled_image_view;

        animgui::synchronized_executor transferer = [&](const std::function<void(vk::CommandBuffer&)>& callback) {
            vk::UniqueCommandBuffer temp = std::move(device
                                                         ->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
                                                             command_pool.get(), vk::CommandBufferLevel::ePrimary, 1 })
                                                         .front());
            temp->begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
            callback(temp.get());
            temp->end();
            queue.submit(vk::SubmitInfo{ 0, nullptr, nullptr, 1, &temp.get(), 0, nullptr });
            queue.waitIdle();
        };

        std::function<void()> draw;
        const auto glfw3_backend = animgui::create_glfw3_backend(window, draw);
        const auto vulkan_backend =
            animgui::create_vulkan_backend(physical_device, device.get(), render_pass.get(), current_command_buffer, sample_count,
                                           sample_shading, transferer, check_vulkan_result);
        const auto stb_font_backend = animgui::create_stb_font_backend(8.0f);
        const auto animator = animgui::create_dummy_animator();
        const auto emitter = animgui::create_builtin_emitter(memory_resource);
        const auto command_optimizer = animgui::create_builtin_command_optimizer();
        const auto image_compactor = animgui::create_builtin_image_compactor(*vulkan_backend, memory_resource);
        auto ctx = animgui::create_animgui_context(*glfw3_backend, *vulkan_backend, *stb_font_backend, *emitter, *animator,
                                                   *command_optimizer, *image_compactor, memory_resource);
        const auto app = animgui::create_demo_application(*ctx);

        auto last = glfwGetTime();
        auto swap_chain_w = 0, swap_chain_h = 0;
        uint32_t current_idx = 0;

        const auto reset_swap_chain = [&](const int w, const int h) {
            device->waitIdle();
            for(auto&& [fence, used] : in_flight_fences) {
                if(used) {
                    check_vulkan_result(device->waitForFences(1, &fence.get(), true, std::numeric_limits<uint64_t>::max()));
                }
            }

            frame_buffers.clear();
            command_buffers.clear();
            render_pass.reset();
            swap_chain_image_views.clear();

            for(uint32_t i = 0; i < max_frames_in_flight; ++i) {
                render_finished_semaphores[i] = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
                image_available_semaphores[i] = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{});
                in_flight_fences[i] = { device->createFenceUnique(vk::FenceCreateInfo{}), false };
            }

            multiple_sampled_image = device->createImageUnique(
                vk::ImageCreateInfo{ {},
                                     vk::ImageType::e2D,
                                     selected_format.format,
                                     vk::Extent3D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 },
                                     1,
                                     1,
                                     sample_count,
                                     vk::ImageTiling::eOptimal,
                                     vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
                                     vk::SharingMode::eExclusive,
                                     0,
                                     nullptr,
                                     vk::ImageLayout::eUndefined });
            const auto requirements = device->getImageMemoryRequirements(multiple_sampled_image.get());

            auto find_memory_type_index = [&](const uint32_t memory_type_bits) {
                constexpr auto property_mask = vk::MemoryPropertyFlagBits::eDeviceLocal;
                for(uint32_t i = 0; i < memory_prop.memoryTypeCount; ++i) {
                    if((memory_type_bits & (1u << i)) &&
                       (memory_prop.memoryTypes[i].propertyFlags & property_mask) == property_mask) {
                        return i;
                    }
                }
                return std::numeric_limits<uint32_t>::max();
            };

            multiple_sampled_image_memory = device->allocateMemoryUnique(
                vk::MemoryAllocateInfo{ requirements.size, find_memory_type_index(requirements.memoryTypeBits) });
            device->bindImageMemory(multiple_sampled_image.get(), multiple_sampled_image_memory.get(), 0);
            multiple_sampled_image_view =
                device->createImageViewUnique(vk::ImageViewCreateInfo{ {},
                                                                       multiple_sampled_image.get(),
                                                                       vk::ImageViewType::e2D,
                                                                       selected_format.format,
                                                                       {},
                                                                       { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } });

            const auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface.get());
            const auto buffers_count = std::min(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount);

            swap_chain = device->createSwapchainKHRUnique(
                vk::SwapchainCreateInfoKHR{ {},
                                            surface.get(),
                                            buffers_count,
                                            selected_format.format,
                                            selected_format.colorSpace,
                                            vk::Extent2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) },
                                            1,
                                            vk::ImageUsageFlagBits::eColorAttachment,
                                            vk::SharingMode::eExclusive,
                                            0,
                                            nullptr,
                                            surface_capabilities.currentTransform,
                                            vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                            present_mode,
                                            true,
                                            swap_chain.get() });
            for(const auto image : device->getSwapchainImagesKHR(swap_chain.get())) {
                swap_chain_image_views.push_back(device->createImageViewUnique(
                    vk::ImageViewCreateInfo{ {},
                                             image,
                                             vk::ImageViewType::e2D,
                                             selected_format.format,
                                             {},
                                             vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } }));
            }

            const vk::AttachmentDescription color[] = { { {},
                                                          selected_format.format,
                                                          sample_count,
                                                          vk::AttachmentLoadOp::eClear,
                                                          vk::AttachmentStoreOp::eStore,
                                                          vk::AttachmentLoadOp::eDontCare,
                                                          vk::AttachmentStoreOp::eDontCare,
                                                          vk::ImageLayout::eUndefined,
                                                          vk::ImageLayout::eColorAttachmentOptimal },
                                                        { {},
                                                          selected_format.format,
                                                          vk::SampleCountFlagBits::e1,
                                                          vk::AttachmentLoadOp::eDontCare,
                                                          vk::AttachmentStoreOp::eStore,
                                                          vk::AttachmentLoadOp::eDontCare,
                                                          vk::AttachmentStoreOp::eDontCare,
                                                          vk::ImageLayout::eUndefined,
                                                          vk::ImageLayout::ePresentSrcKHR } };
            const vk::AttachmentReference color_ref{ 0, vk::ImageLayout::eColorAttachmentOptimal };
            const vk::AttachmentReference color_resolve_ref{ 1, vk::ImageLayout::eColorAttachmentOptimal };
            const vk::SubpassDescription sub_pass{
                {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &color_ref, &color_resolve_ref, nullptr, 0, nullptr
            };
            const vk::SubpassDependency dependency{ VK_SUBPASS_EXTERNAL,
                                                    0,
                                                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                                    {},
                                                    vk::AccessFlagBits::eColorAttachmentWrite };
            render_pass = device->createRenderPassUnique(
                vk::RenderPassCreateInfo{ {}, static_cast<uint32_t>(std::size(color)), color, 1, &sub_pass, 1, &dependency });
            for(auto&& image_view : swap_chain_image_views) {
                const vk::ImageView attachments[] = { multiple_sampled_image_view.get(), image_view.get() };
                frame_buffers.push_back(
                    device->createFramebufferUnique(vk::FramebufferCreateInfo{ {},
                                                                               render_pass.get(),
                                                                               static_cast<uint32_t>(std::size(attachments)),
                                                                               attachments,
                                                                               static_cast<uint32_t>(w),
                                                                               static_cast<uint32_t>(h),
                                                                               1 }));
            }
            command_buffers = device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
                command_pool.get(), vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(frame_buffers.size()) });
            current_command_buffer = command_buffers.front().get();
            images_in_flight.clear();
            images_in_flight.resize(frame_buffers.size(), vk::Fence{});
        };

        draw = [&] {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            if(w == 0 || h == 0)
                return;

            if(w != swap_chain_w || h != swap_chain_h) {
                swap_chain_w = w;
                swap_chain_h = h;
                reset_swap_chain(w, h);
                return;
            }

            const auto current = glfwGetTime();
            const auto delta_t = static_cast<float>(current - last);
            last = current;

            int window_w, window_h;
            glfwGetWindowSize(window, &window_w, &window_h);

            ctx->new_frame(window_w, window_h, delta_t, [&](animgui::canvas& canvas_root) { app->render(canvas_root); });

            auto&& [fence, used] = in_flight_fences[current_idx];

            if(used)
                check_vulkan_result(device->waitForFences(1, &fence.get(), true, std::numeric_limits<uint64_t>::max()));

            const auto [next_res, image_idx] = device->acquireNextImageKHR(swap_chain.get(), std::numeric_limits<uint64_t>::max(),
                                                                           image_available_semaphores[current_idx].get());
            if(next_res != vk::Result::eSuccess) {
                // reset_swap_chain(w, h);
                return;
            }

            current_command_buffer = command_buffers[image_idx].get();
            current_command_buffer.begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
            const vk::ClearValue clear_color{ vk::ClearColorValue{ std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } } };
            current_command_buffer.beginRenderPass(
                vk::RenderPassBeginInfo{ render_pass.get(), frame_buffers[image_idx].get(),
                                         vk::Rect2D{ {}, vk::Extent2D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) } }, 1,
                                         &clear_color },
                vk::SubpassContents::eInline);
            vulkan_backend->emit(animgui::uvec2{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });
            current_command_buffer.endRenderPass();
            current_command_buffer.end();

            if(images_in_flight[image_idx] != static_cast<vk::Fence>(nullptr)) {
                check_vulkan_result(
                    device->waitForFences(1, &images_in_flight[image_idx], true, std::numeric_limits<uint64_t>::max()));
            }
            images_in_flight[image_idx] = fence.get();
            used = true;

            check_vulkan_result(device->resetFences(1, &fence.get()));
            const vk::PipelineStageFlags stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            const vk::SubmitInfo submit_info{
                1, &image_available_semaphores[current_idx].get(), &stage, 1, &current_command_buffer,
                1, &render_finished_semaphores[current_idx].get()
            };
            check_vulkan_result(queue.submit(1, &submit_info, fence.get()));

            const vk::PresentInfoKHR present_info{ 1, &render_finished_semaphores[current_idx].get(), 1, &swap_chain.get(),
                                                   &image_idx };
            if(static_cast<vk::Result>(vkQueuePresentKHR(queue, reinterpret_cast<const VkPresentInfoKHR*>(&present_info))) !=
               vk::Result::eSuccess) {
                // reset_swap_chain(w, h);
                return;
            }
            current_idx = (current_idx + 1) % max_frames_in_flight;
        };

        while(!glfwWindowShouldClose(window)) {
            glfw3_backend->new_frame();
            draw();
        }

        device->waitIdle();
    } catch(const std::exception& error) {
        std::cerr << error.what() << std::endl;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
