// SPDX-License-Identifier: MIT

#include <animgui/backends/vulkan.hpp>
#include <animgui/core/render_backend.hpp>
#include <queue>

// TODO: https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/

namespace animgui {

    static const uint32_t vert_spv[] =
#include "vert.spv.hpp"
        ;

    static const uint32_t frag_spv[] =
#include "frag.spv.hpp"
        ;

    using buffer_pair = std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>;

    [[nodiscard]] uint32_t find_memory_index(const vk::PhysicalDeviceMemoryProperties& memory_prop,
                                             const uint32_t memory_type_bits, const vk::MemoryPropertyFlags property_mask) {
        for(uint32_t i = 0; i < memory_prop.memoryTypeCount; ++i) {
            if((memory_type_bits & (1u << i)) && (memory_prop.memoryTypes[i].propertyFlags & property_mask) == property_mask) {
                return i;
            }
        }
        return std::numeric_limits<uint32_t>::max();
    }

    [[nodiscard]] buffer_pair allocate_buffer(vk::Device& device, vk::PhysicalDeviceMemoryProperties& memory_prop,
                                              const size_t size, const vk::BufferUsageFlags usage,
                                              const vk::MemoryPropertyFlags property_flags) {
        vk::UniqueBuffer buffer = device.createBufferUnique(vk::BufferCreateInfo{ {}, static_cast<uint32_t>(size), usage });
        const auto requirements = device.getBufferMemoryRequirements(buffer.get());
        vk::UniqueDeviceMemory memory = device.allocateMemoryUnique(vk::MemoryAllocateInfo{
            requirements.size, find_memory_index(memory_prop, requirements.memoryTypeBits, property_flags) });
        device.bindBufferMemory(buffer.get(), memory.get(), 0);
        return std::make_pair(std::move(buffer), std::move(memory));
    }

    void update_buffer(vk::Device& device, const buffer_pair& buffer, const void* src, const size_t size) {
        const auto ptr = device.mapMemory(buffer.second.get(), 0, size, {});
        memcpy(ptr, src, size);
        device.unmapMemory(buffer.second.get());
    }

    // TODO: mipmap
    class texture_impl final : public texture {
        uvec2 m_size;
        channel m_channel;
        vk::Device& m_device;
        vk::PhysicalDeviceMemoryProperties& m_memory_prop;
        const synchronized_executor& m_synchronized_transfer;

        std::function<void(vk::Result)>& m_error_report;
        vk::UniqueImage m_image;
        vk::UniqueDeviceMemory m_image_memory;
        vk::UniqueImageView m_image_view;
        bool m_dirty = false;
        bool m_first_update = true;

        static vk::Format get_format(const channel channel) noexcept {
            return channel == channel::alpha ? vk::Format::eR8Unorm :
                                               (channel == channel::rgb ? vk::Format::eR8G8B8Unorm : vk::Format::eR8G8B8A8Unorm);
        }

        static uint32_t get_pixel_size(const channel channel) noexcept {
            return channel == channel::alpha ? 1 : (channel == channel::rgb ? 3 : 4);
        }

        void check_vulkan_result(const vk::Result res) const {
            if(res != vk::Result::eSuccess) {
                m_error_report(res);
            }
        }

        void create_image_view(const vk::Image image, const vk::Format format, const uint32_t mip_level) {
            const auto color_mapping = m_channel == channel::alpha ? vk::ComponentSwizzle::eOne : vk::ComponentSwizzle::eIdentity;
            const auto alpha_mapping = m_channel == channel::alpha ?
                vk::ComponentSwizzle::eR :
                (m_channel == channel::rgb ? vk::ComponentSwizzle::eOne : vk::ComponentSwizzle::eIdentity);
            m_image_view = m_device.createImageViewUnique(
                vk::ImageViewCreateInfo{ {},
                                         image,
                                         vk::ImageViewType::e2D,
                                         format,
                                         vk::ComponentMapping{ color_mapping, color_mapping, color_mapping, alpha_mapping },
                                         vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } });
        }

    public:
        texture_impl(vk::Device& device, const synchronized_executor& synchronized_transfer,
                     std::function<void(vk::Result)>& error_report, vk::PhysicalDeviceMemoryProperties& memory_prop,
                     const uvec2 size, const channel image_channel)
            : m_size{ size }, m_channel{ image_channel }, m_device{ device }, m_memory_prop{ memory_prop },
              m_synchronized_transfer{ synchronized_transfer }, m_error_report{ error_report } {
            const auto format = get_format(image_channel);
            const auto mip_level = calculate_mipmap_level(size);

            m_image = device.createImageUnique(
                vk::ImageCreateInfo{ {},
                                     vk::ImageType::e2D,
                                     format,
                                     vk::Extent3D{ size.x, size.y, 1 },
                                     1,
                                     1,
                                     vk::SampleCountFlagBits::e1,
                                     vk::ImageTiling::eOptimal,
                                     vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
                                     vk::SharingMode::eExclusive,
                                     {},
                                     {},
                                     vk::ImageLayout::eUndefined });
            const auto requirements = device.getImageMemoryRequirements(m_image.get());
            m_image_memory = device.allocateMemoryUnique(vk::MemoryAllocateInfo{
                requirements.size,
                find_memory_index(memory_prop, requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal) });
            device.bindImageMemory(m_image.get(), m_image_memory.get(), 0);
            create_image_view(m_image.get(), format, mip_level);
        }
        texture_impl(vk::Device& device, const synchronized_executor& synchronized_transfer,
                     std::function<void(vk::Result)>& error_report, vk::PhysicalDeviceMemoryProperties& memory_prop,
                     const vk::Image image, const uvec2 size, const channel image_channel)
            : m_size{ size }, m_channel{ image_channel }, m_device{ device }, m_memory_prop{ memory_prop },
              m_synchronized_transfer{ synchronized_transfer }, m_error_report{ error_report } {
            const auto format = get_format(image_channel);
            const auto mip_level = calculate_mipmap_level(size);
            create_image_view(image, format, mip_level);
        }

        void update_texture(const uvec2 offset, const image_desc& image) override {
            if(image.channels != m_channel)
                throw std::runtime_error{ "mismatched channel" };
            if(image.size.x == 0 || image.size.y == 0)
                return;
            // TODO: shared staging buffer
            const auto size = image.size.x * image.size.y * get_pixel_size(m_channel);
            const auto temp_staging_buffer =
                allocate_buffer(m_device, m_memory_prop, size, vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);
            update_buffer(m_device, temp_staging_buffer, image.data, size);

            m_synchronized_transfer([&](vk::CommandBuffer& cmd) {
                const vk::ImageMemoryBarrier enter{
                    m_first_update ? vk::AccessFlagBits::eNoneKHR : vk::AccessFlagBits::eShaderRead,
                    vk::AccessFlagBits::eTransferWrite,
                    m_first_update ? vk::ImageLayout::eUndefined : vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::ImageLayout::eTransferDstOptimal,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    m_image.get(),
                    vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
                };
                cmd.pipelineBarrier(m_first_update ? vk::PipelineStageFlagBits::eTopOfPipe :
                                                     vk::PipelineStageFlagBits::eFragmentShader,
                                    vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0, nullptr, 1, &enter);

                const vk::BufferImageCopy region{ 0,
                                                  0,
                                                  0,
                                                  { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
                                                  vk::Offset3D{ static_cast<int32_t>(offset.x), static_cast<int32_t>(offset.y),
                                                                0 },
                                                  vk::Extent3D{ image.size.x, image.size.y, 1 } };
                cmd.copyBufferToImage(temp_staging_buffer.first.get(), m_image.get(), vk::ImageLayout::eTransferDstOptimal, 1,
                                      &region);

                const vk::ImageMemoryBarrier exit{ vk::AccessFlagBits::eTransferWrite,
                                                   vk::AccessFlagBits::eShaderRead,
                                                   vk::ImageLayout::eTransferDstOptimal,
                                                   vk::ImageLayout::eShaderReadOnlyOptimal,
                                                   VK_QUEUE_FAMILY_IGNORED,
                                                   VK_QUEUE_FAMILY_IGNORED,
                                                   m_image.get(),
                                                   vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
                cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, 0,
                                    nullptr, 0, nullptr, 1, &exit);
            });

            m_first_update = false;
            m_dirty = true;
        }

        void generate_mipmap() override {
            if(m_dirty) {
                m_dirty = false;
            }
        }

        [[nodiscard]] uvec2 texture_size() const noexcept override {
            return m_size;
        }
        [[nodiscard]] channel channels() const noexcept override {
            return m_channel;
        }
        [[nodiscard]] uint64_t native_handle() const noexcept override {
            return reinterpret_cast<uint64_t>(static_cast<VkImageView>(m_image_view.get()));
        }
    };

    class render_backend_impl final : public render_backend {
        static constexpr uint32_t max_textures = 32;

        vk::Device& m_device;
        vk::RenderPass& m_render_pass;
        vk::CommandBuffer& m_command_buffer;
        synchronized_executor m_synchronized_transfer;

        vk::UniqueShaderModule m_vert;
        vk::UniqueShaderModule m_frag;
        vk::UniqueSampler m_sampler;
        vk::UniqueDescriptorSetLayout m_descriptor_set_layout;
        vk::UniqueDescriptorPool m_descriptor_pool;

        std::unordered_map<VkImageView, vk::DescriptorSet> m_texture_binding;
        std::queue<std::pair<VkImageView, vk::UniqueDescriptorSet>> m_binding_queue;
        vk::DescriptorSet bind_texture(VkImageView image_view) {
            if(const auto iter = m_texture_binding.find(image_view); iter != m_texture_binding.cend())
                return iter->second;
            auto [old, set] = std::move(m_binding_queue.front());
            m_binding_queue.pop();
            m_texture_binding.erase(old);
            const vk::DescriptorImageInfo image_info{ m_sampler.get(), { image_view }, vk::ImageLayout::eShaderReadOnlyOptimal };
            const vk::WriteDescriptorSet write_descriptor_set{
                set.get(), 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_info, nullptr, nullptr
            };
            m_device.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);

            const auto res = set.get();
            m_binding_queue.push(std::make_pair(image_view, std::move(set)));
            m_texture_binding.insert(std::make_pair(image_view, res));
            return res;
        }

        vk::UniquePipelineLayout m_pipeline_layout;
        vk::UniquePipeline m_pipeline;

        uvec2 m_window_size = {};
        std::pmr::vector<command> m_command_list;

        uint64_t m_render_time = 0;
        uvec2 m_last_screen_size = {};

        std::function<void(vk::Result)> m_error_report;

        void check_result(const vk::Result res) const {
            if(res != vk::Result::eSuccess) {
                m_error_report(res);
            }
        }

        void build_context(const uvec2 screen_size) {
            m_device.waitIdle();

            m_pipeline.reset();
            m_pipeline_layout.reset();

            const vec2 window_size = { static_cast<float>(m_window_size.x), static_cast<float>(m_window_size.y) };
            const vk::SpecializationMapEntry entries[] = { { 0, 0, sizeof(float) }, { 1, sizeof(float), sizeof(float) } };
            vk::SpecializationInfo specialization_info{ static_cast<uint32_t>(std::size(entries)), entries, sizeof(vec2),
                                                        &window_size };

            const vk::PipelineShaderStageCreateInfo shader_desc[2] = {
                { {}, vk::ShaderStageFlagBits::eVertex, m_vert.get(), "main", &specialization_info },
                { {}, vk::ShaderStageFlagBits::eFragment, m_frag.get(), "main" }
            };
            vk::VertexInputBindingDescription vertex_input_binding{ 0, sizeof(vertex), vk::VertexInputRate::eVertex };
            vk::VertexInputAttributeDescription attributes[3] = {
                { 0, 0, vk::Format::eR32G32Sfloat, offset_u32(&vertex::pos) },
                { 1, 0, vk::Format::eR32G32Sfloat, offset_u32(&vertex::tex_coord) },
                { 2, 0, vk::Format::eR32G32B32A32Sfloat, offset_u32(&vertex::color) }
            };
            vk::PipelineVertexInputStateCreateInfo vertex_input{
                {}, 1, &vertex_input_binding, static_cast<uint32_t>(std::size(attributes)), attributes
            };
            vk::PipelineInputAssemblyStateCreateInfo input_assembly{ {}, vk::PrimitiveTopology::eTriangleList, false };
            vk::PipelineRasterizationStateCreateInfo rasterization_state{ {},
                                                                          false,
                                                                          false,
                                                                          vk::PolygonMode::eFill,
                                                                          vk::CullModeFlagBits::eNone,
                                                                          vk::FrontFace::eCounterClockwise,
                                                                          false,
                                                                          0.0f,
                                                                          0.0f,
                                                                          0.0f,
                                                                          1.0f };
            vk::PipelineColorBlendAttachmentState color_blend_attachment_state{ true,
                                                                                vk::BlendFactor::eSrcAlpha,
                                                                                vk::BlendFactor::eOneMinusSrcAlpha,
                                                                                vk::BlendOp::eAdd,
                                                                                vk::BlendFactor::eOne,
                                                                                vk::BlendFactor::eOneMinusSrcAlpha,
                                                                                vk::BlendOp::eAdd,
                                                                                vk::ColorComponentFlagBits::eR |
                                                                                    vk::ColorComponentFlagBits::eG |
                                                                                    vk::ColorComponentFlagBits::eB |
                                                                                    vk::ColorComponentFlagBits::eA };
            vk::PipelineMultisampleStateCreateInfo multiple_sampling_state{};  // TODO: MSAA
            vk::PipelineColorBlendStateCreateInfo color_blend_state{
                {}, false, vk::LogicOp::eNoOp, 1, &color_blend_attachment_state
            };
            vk::Viewport viewport{ 0.0f, 0.0f, static_cast<float>(screen_size.x), static_cast<float>(screen_size.y), 0.0f, 1.0f };
            vk::Rect2D scissor{};
            vk::PipelineViewportStateCreateInfo viewport_state{ {}, 1, &viewport, 1, &scissor };
            vk::DynamicState dynamic_state[] = { vk::DynamicState::eScissor };
            vk::PipelineDynamicStateCreateInfo dynamic_state_desc{ {},
                                                                   static_cast<uint32_t>(std::size(dynamic_state)),
                                                                   dynamic_state };
            const auto descriptor_layout = m_descriptor_set_layout.get();
            m_pipeline_layout = m_device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo{ {}, 1, &descriptor_layout });

            m_pipeline = m_device
                             .createGraphicsPipelineUnique({},
                                                           vk::GraphicsPipelineCreateInfo{
                                                               {},
                                                               2,
                                                               static_cast<const vk::PipelineShaderStageCreateInfo*>(shader_desc),
                                                               &vertex_input,
                                                               &input_assembly,
                                                               nullptr,
                                                               &viewport_state,
                                                               &rasterization_state,
                                                               &multiple_sampling_state,
                                                               nullptr,
                                                               &color_blend_state,
                                                               &dynamic_state_desc,
                                                               m_pipeline_layout.get(),
                                                               m_render_pass,
                                                               0,
                                                               nullptr,
                                                               0 })
                             .value;
        }

        bool m_dirty = false;
        bool m_scissor_restricted = false;
        VkImageView m_bind_tex = nullptr;

        void make_dirty() {
            m_dirty = true;
            m_scissor_restricted = true;
            m_bind_tex = reinterpret_cast<VkImageView>(std::numeric_limits<size_t>::max());
        }

        vk::PhysicalDeviceMemoryProperties m_memory_prop;

        buffer_pair m_vertex_buffer;
        size_t m_vertex_buffer_size = 0;

        void update_vertex_buffer(const std::pmr::vector<vertex>& vertices) {
            if(m_vertex_buffer_size < vertices.size()) {
                m_vertex_buffer_size = std::max(m_vertex_buffer_size, static_cast<size_t>(1024)) * 2;
                m_vertex_buffer = allocate_buffer(
                    m_device, m_memory_prop, m_vertex_buffer_size * sizeof(vertex), vk::BufferUsageFlagBits::eVertexBuffer,
                    vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);
            }

            update_buffer(m_device, m_vertex_buffer, vertices.data(), vertices.size() * sizeof(vertex));
        }

        std::shared_ptr<texture> m_empty;

        void emit(const primitives& primitives, uint32_t& vertices_offset) {
            vk::CommandBuffer& cmd = m_command_buffer;
            auto&& [type, vertices_count, tex, point_line_size] = primitives;

            if(m_dirty) {
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
                vk::Buffer buffer = m_vertex_buffer.first.get();
                vk::DeviceSize offset = 0;
                cmd.bindVertexBuffers(0, 1, &buffer, &offset);

                m_dirty = false;
            }

            if(auto cmd_tex = tex ? tex.get() : m_empty.get();
               reinterpret_cast<VkImageView>(cmd_tex->native_handle()) != m_bind_tex) {
                cmd_tex->generate_mipmap();
                m_bind_tex = reinterpret_cast<VkImageView>(cmd_tex->native_handle());
                const auto descriptor_set = bind_texture(m_bind_tex);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline_layout.get(), 0, 1, &descriptor_set, 0,
                                       nullptr);
            }

            cmd.draw(vertices_count, 1, vertices_offset, 0);
            vertices_offset += vertices_count;
        }

        void emit(const native_callback& callback, uint32_t&) {
            callback();
            make_dirty();
        }

    public:
        render_backend_impl(vk::PhysicalDevice& physical_device, vk::Device& device, vk::RenderPass& render_pass,
                            vk::CommandBuffer& command_buffer, synchronized_executor synchronized_transfer,
                            std::function<void(vk::Result)> error_report)
            : m_device{ device }, m_render_pass{ render_pass }, m_command_buffer{ command_buffer },
              m_synchronized_transfer{ std::move(synchronized_transfer) }, m_error_report{ std::move(error_report) },
              m_memory_prop{ physical_device.getMemoryProperties() } {

            m_vert = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{ {}, sizeof(vert_spv), vert_spv });
            m_frag = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{ {}, sizeof(frag_spv), frag_spv });
            m_sampler = device.createSamplerUnique(
                vk::SamplerCreateInfo{ {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear });
            const vk::DescriptorSetLayoutBinding binding[] = { { 0, vk::DescriptorType::eCombinedImageSampler, 1,
                                                                 vk::ShaderStageFlagBits::eFragment, nullptr } };
            m_descriptor_set_layout = device.createDescriptorSetLayoutUnique(
                vk::DescriptorSetLayoutCreateInfo{ {}, static_cast<uint32_t>(std::size(binding)), binding });
            vk::DescriptorPoolSize pool_size[]{ { vk::DescriptorType::eCombinedImageSampler, max_textures } };
            m_descriptor_pool = device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
                max_textures, static_cast<uint32_t>(std::size(pool_size)), pool_size });
            vk::DescriptorSetLayout set_layout[max_textures];
            std::fill(std::begin(set_layout), std::end(set_layout), m_descriptor_set_layout.get());
            for(auto&& set : m_device.allocateDescriptorSetsUnique(
                    vk::DescriptorSetAllocateInfo{ m_descriptor_pool.get(), max_textures, set_layout })) {
                m_binding_queue.push(
                    std::make_pair(reinterpret_cast<VkImageView>(std::numeric_limits<size_t>::max()), std::move(set)));
            }

            m_empty = create_texture(uvec2{ 1, 1 }, channel::rgba);
            uint8_t data[4] = { 255, 255, 255, 255 };
            m_empty->update_texture(uvec2{ 0, 0 }, image_desc{ { 1, 1 }, channel::rgba, data });
        }
        void update_command_list(const uvec2 window_size, command_queue command_list) override {
            m_window_size = window_size;

            update_vertex_buffer(command_list.vertices);

            m_command_list.clear();
            m_command_list.reserve(command_list.commands.size());

            for(auto&& command : command_list.commands)
                m_command_list.push_back(std::move(command));
        }
        std::shared_ptr<texture> create_texture(uvec2 size, channel channels) override {
            return std::make_shared<texture_impl>(m_device, m_synchronized_transfer, m_error_report, m_memory_prop, size,
                                                  channels);
        }
        std::shared_ptr<texture> create_texture_from_native_handle(const uint64_t handle, uvec2 size, channel channels) override {
            return std::make_shared<texture_impl>(m_device, m_synchronized_transfer, m_error_report, m_memory_prop,
                                                  reinterpret_cast<VkImage>(handle), size, channels);
        }

        void emit(const uvec2 screen_size) override {
            if(m_last_screen_size != screen_size) {
                build_context(screen_size);
                m_last_screen_size = screen_size;
            }

            const auto tp1 = current_time();

            make_dirty();

            uint32_t vertices_offset = 0;

            vk::CommandBuffer& cmd = m_command_buffer;

            const vec2 scale = { static_cast<float>(screen_size.x) / m_window_size.x,
                                 static_cast<float>(screen_size.y) / m_window_size.y };

            // ReSharper disable once CppUseStructuredBinding
            for(auto&& command : m_command_list) {
                if(command.clip.has_value()) {
                    // ReSharper disable once CppUseStructuredBinding
                    auto&& clip = command.clip.value();
                    const auto left = static_cast<int32_t>(std::floor(clip.left * scale.x));
                    const auto right = static_cast<int32_t>(std::ceil(clip.right * scale.x));
                    const auto bottom = static_cast<int32_t>(std::ceil(clip.bottom * scale.y));
                    const auto top = static_cast<int32_t>(std::floor(clip.top * scale.y));
                    const vk::Rect2D scissor{ { left, top },
                                              { static_cast<uint32_t>(right - left), static_cast<uint32_t>(bottom - top) } };
                    cmd.setScissor(0, 1, &scissor);
                    m_scissor_restricted = true;
                } else if(m_scissor_restricted) {
                    const vk::Rect2D scissor{ { 0, 0 },
                                              { static_cast<uint32_t>(screen_size.x), static_cast<uint32_t>(screen_size.y) } };
                    cmd.setScissor(0, 1, &scissor);
                    m_scissor_restricted = false;
                }

                std::visit([&](auto&& item) { emit(item, vertices_offset); }, command.desc);
            }

            const auto tp2 = current_time();
            m_render_time = tp2 - tp1;
        }
        [[nodiscard]] uint64_t render_time() const noexcept override {
            return m_render_time;
        }
        [[nodiscard]] primitive_type supported_primitives() const noexcept override {
            return primitive_type::triangles;
        }
    };

    ANIMGUI_API std::shared_ptr<render_backend> create_vulkan_backend(vk::PhysicalDevice& physical_device, vk::Device& device,
                                                                      vk::RenderPass& render_pass,
                                                                      vk::CommandBuffer& command_buffer,
                                                                      synchronized_executor synchronized_transfer,
                                                                      std::function<void(vk::Result)> error_report) {
        return std::make_shared<render_backend_impl>(physical_device, device, render_pass, command_buffer,
                                                     std::move(synchronized_transfer), std::move(error_report));
    }

}  // namespace animgui
