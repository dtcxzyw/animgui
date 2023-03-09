// SPDX-License-Identifier: MIT

#define NOMINMAX
#include "hlsl_shaders.hpp"
#include <animgui/backends/d3d12.hpp>
#include <animgui/core/render_backend.hpp>
#include <cassert>
#include <cmath>
#include <cstring>
#include <directx/d3dcommon.h>
#include <d3dcompiler.h>
#include <directx/d3dx12.h>
#include <queue>
#include <utility>

using Microsoft::WRL::ComPtr;

namespace animgui {
    constexpr auto align(const size_t origin, const size_t alignment) noexcept {
        return ((origin + alignment - 1) / alignment) * alignment;
    }

    // TODO: Rasterizer Order Views?
    static DXGI_FORMAT get_format(const channel channel) noexcept {
        return channel == channel::alpha ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    class texture_impl final : public texture {
        ID3D12Resource* m_texture;
        channel m_channel;
        uvec2 m_size;
        bool m_own;
        bool m_dirty;
        ID3D12Device* m_device;
        const synchronized_executor& m_synchronized_transferer;
        const std::function<void(long)>& m_error_checker;

        void check_d3d_error(const HRESULT res) const {
            if(res != S_OK)
                m_error_checker(res);
        }

    public:
        explicit texture_impl(const texture_impl&) = delete;
        explicit texture_impl(texture_impl&&) = delete;
        texture_impl& operator=(const texture_impl&) = delete;
        texture_impl& operator=(texture_impl&&) = delete;

        texture_impl(ID3D12Device* device, const synchronized_executor& synchronized_transferer, const channel channel,
                     const uvec2 size, const std::function<void(long)>& error_checker)
            : m_texture{ nullptr }, m_channel{ channel }, m_size{ size }, m_own{ true }, m_dirty{ false }, m_device{ device },
              m_synchronized_transferer{ synchronized_transferer }, m_error_checker{ error_checker } {
            const auto resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                get_format(channel), size.x, size.y, 1,
                // static_cast<UINT16>(calculate_mipmap_level(size)) //TODO: mipmap generation using compute shader
                1);
            const auto heap_properties = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT };
            check_d3d_error(device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
                                                            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_texture)));
        }

        texture_impl(ID3D12Resource* handle, ID3D12Device* device, const synchronized_executor& synchronized_transferer,
                     const channel channel, const uvec2 size, const std::function<void(long)>& error_checker)
            : m_texture{ handle }, m_channel{ channel }, m_size{ size }, m_own(false), m_dirty{ false }, m_device{ device },
              m_synchronized_transferer{ synchronized_transferer }, m_error_checker{ error_checker } {}

        ~texture_impl() override {
            if(m_own) {
                m_texture->Release();
            }
        }
        void generate_mipmap() override {
            if(!m_dirty)
                return;

            m_dirty = false;
        }

        void update_texture(const uvec2 offset, const image_desc& image) override {
            if(image.channels != m_channel)
                throw std::runtime_error{ "mismatched channel" };
            if(image.size.x == 0 || image.size.y == 0)
                return;

            auto [size, channels, data] = image;

            const auto size_dst = m_channel == channel::alpha ? 1 : 4;
            const D3D12_BOX box{ 0, 0, 0, size.x, size.y, 1 };

            D3D12_SUBRESOURCE_FOOTPRINT pitched_desc;
            pitched_desc.Format = get_format(m_channel);
            pitched_desc.Width = size.x;
            pitched_desc.Height = size.y;
            pitched_desc.Depth = 1;
            pitched_desc.RowPitch = align(size.x * size_dst, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

            const auto total_size = pitched_desc.RowPitch * size.y;

            // TODO: shared staging buffer
            const auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(total_size);
            const auto heap_properties = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_UPLOAD };
            ComPtr<ID3D12Resource> staging_buffer;
            m_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
                                              D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                              IID_PPV_ARGS(staging_buffer.GetAddressOf()));
            const D3D12_RANGE discard{ 0, 0 };
            void* ptr;
            check_d3d_error(staging_buffer->Map(0, &discard, &ptr));
            const auto typed_ptr = static_cast<std::byte*>(ptr);

            if(channels == channel::rgb) {
                for(uint32_t y = 0; y < size.y; ++y) {
                    for(uint32_t x = 0; x < size.x; ++x) {
                        const auto base = typed_ptr + y * pitched_desc.RowPitch + x * 4;
                        memcpy(base, static_cast<const std::byte*>(data) + 3 * size.x * y + x * 3, 3);
                        base[3] = static_cast<std::byte>(0);
                    }
                }
            } else {
                for(uint32_t y = 0; y < size.y; ++y) {
                    memcpy(typed_ptr + y * pitched_desc.RowPitch, static_cast<const std::byte*>(data) + size_dst * size.x * y,
                           size_dst * size.x);
                }
            }

            const D3D12_RANGE range{ 0, total_size };
            staging_buffer->Unmap(0, &range);

            m_synchronized_transferer([&](ID3D12GraphicsCommandList* cmd) {
                const D3D12_TEXTURE_COPY_LOCATION src{ staging_buffer.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                                                       D3D12_PLACED_SUBRESOURCE_FOOTPRINT{ 0, pitched_desc } };
                const D3D12_TEXTURE_COPY_LOCATION dst{ m_texture, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, {} };
                cmd->CopyTextureRegion(&dst, offset.x, offset.y, 0, &src, &box);
            });

            m_dirty = true;
        }

        [[nodiscard]] uvec2 texture_size() const noexcept override {
            return m_size;
        }

        [[nodiscard]] channel channels() const noexcept override {
            return m_channel;
        }

        [[nodiscard]] uint64_t native_handle() const noexcept override {
            return reinterpret_cast<uint64_t>(m_texture);
        }
    };

    class d3d12_backend final : public render_backend {
        static constexpr uint32_t max_textures = 32;

        ID3D12Device* m_device;
        ID3D12GraphicsCommandList* m_command_list;
        synchronized_executor m_synchronized_transferer;
        std::function<void(HRESULT)> m_error_checker;

        void check_d3d_error(const HRESULT res) const {
            if(res != S_OK)
                m_error_checker(res);
        }

        ComPtr<ID3D12PipelineState> m_pipeline_state;
        ComPtr<ID3D12RootSignature> m_root_signature;
        ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;
        ComPtr<ID3D12DescriptorHeap> m_sampler_descriptor_heap;
        D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_descriptor_handle{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_descriptor_handle{};
        uint32_t m_descriptor_increment_size = 0;
        ComPtr<ID3D12Resource> m_constant_buffer;
        std::queue<std::pair<ComPtr<ID3D12Resource>, int32_t>> m_descriptor_queue;

        struct resource_hasher final {
            uint64_t operator()(const ComPtr<ID3D12Resource>& ref) const {
                return std::hash<void*>()(ref.Get());
            }
        };

        std::pmr::unordered_map<ComPtr<ID3D12Resource>, int32_t, resource_hasher> m_texture_binding;

        ComPtr<ID3D12Resource> m_vertex_buffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view = {};
        size_t m_vertex_buffer_size = 0;

        void update_vertex_buffer(const std::pmr::vector<vertex>& vertices) {

            if(m_vertex_buffer_size < vertices.size()) {
                if(m_vertex_buffer)
                    m_vertex_buffer->Release();
                m_vertex_buffer_size = std::max(m_vertex_buffer_size, static_cast<size_t>(1024)) * 2;
                const auto heap_properties = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_UPLOAD };
                const auto vertex_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertex) * m_vertex_buffer_size);

                check_d3d_error(m_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &vertex_buffer_desc,
                                                                  D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
                                                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                  nullptr, IID_PPV_ARGS(m_vertex_buffer.GetAddressOf())));
                m_vertex_buffer_view = { m_vertex_buffer->GetGPUVirtualAddress(),
                                         static_cast<uint32_t>(sizeof(vertex) * m_vertex_buffer_size), sizeof(vertex) };
            }

            const auto size = vertices.size() * sizeof(vertex);
            void* ptr;
            const D3D12_RANGE discard{ 0, 0 };
            check_d3d_error(m_vertex_buffer->Map(0, &discard, &ptr));
            memcpy(ptr, vertices.data(), size);
            const D3D12_RANGE range{ 0, size };
            m_vertex_buffer->Unmap(0, &range);
        }

        bool m_dirty = false;
        bool m_scissor_restricted = false;

        void make_dirty() {
            m_dirty = true;
            m_scissor_restricted = true;
        }

        void emit(const native_callback& callback, uint32_t&) {
            callback();
            make_dirty();
        }

        static D3D12_PRIMITIVE_TOPOLOGY get_primitive_type(const primitive_type type) noexcept {
            // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement CppIncompleteSwitchStatement
            switch(type) {  // NOLINT(clang-diagnostic-switch)
                case primitive_type::triangles:
                    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
                case primitive_type::triangle_strip:
                    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            }
            return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(0);
        }

        void emit(const primitives& primitives, uint32_t& vertices_offset) {
            auto&& [type, vertices_count, tex, point_line_size] = primitives;

            if(m_dirty) {
                m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
                m_command_list->SetPipelineState(m_pipeline_state.Get());
                m_command_list->IASetVertexBuffers(0, 1, &m_vertex_buffer_view);
                const auto heaps = { m_descriptor_heap.Get(), m_sampler_descriptor_heap.Get() };
                m_command_list->SetDescriptorHeaps(heaps.size(), heaps.begin());
                m_command_list->SetGraphicsRootDescriptorTable(1,
                                                               m_sampler_descriptor_heap->GetGPUDescriptorHandleForHeapStart());

                m_dirty = false;
            }

            m_command_list->IASetPrimitiveTopology(get_primitive_type(type));

            if(tex) {
                const auto texture_handle = reinterpret_cast<ID3D12Resource*>(tex->native_handle());
                tex->generate_mipmap();

                int32_t offset;
                if(const auto iter = m_texture_binding.find(texture_handle); iter != m_texture_binding.cend()) {
                    offset = iter->second;
                } else {
                    const auto [old, idx] = m_descriptor_queue.front();
                    m_descriptor_queue.pop();
                    m_texture_binding.erase(old);

                    // TODO: mapping
                    D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{
                        get_format(tex->channels()), D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, {}
                    };
                    shader_resource_view_desc.Texture2D = { 0, 1, 0, 0.0f };

                    m_device->CreateShaderResourceView(
                        texture_handle, &shader_resource_view_desc,
                        CD3DX12_CPU_DESCRIPTOR_HANDLE{ m_cpu_descriptor_handle, idx, m_descriptor_increment_size });
                    m_descriptor_queue.push({ texture_handle, idx });
                    m_texture_binding.insert({ texture_handle, idx });
                    offset = idx;
                }

                m_command_list->SetGraphicsRootDescriptorTable(
                    0,
                    CD3DX12_GPU_DESCRIPTOR_HANDLE{ m_gpu_descriptor_handle, tex->channels() == channel::alpha ? 1 : 0,
                                                   m_descriptor_increment_size });
                m_command_list->SetGraphicsRootDescriptorTable(
                    2, CD3DX12_GPU_DESCRIPTOR_HANDLE{ m_gpu_descriptor_handle, offset, m_descriptor_increment_size });
            } else {
                m_command_list->SetGraphicsRootDescriptorTable(
                    0, CD3DX12_GPU_DESCRIPTOR_HANDLE{ m_gpu_descriptor_handle, 2, m_descriptor_increment_size });
            }

            m_command_list->DrawInstanced(vertices_count, 1, vertices_offset, 0);
            vertices_offset += vertices_count;
        }

        uint64_t m_render_time = 0;
        uvec2 m_window_size = {};
        std::pmr::vector<command> m_command_buffer;

    public:
        d3d12_backend(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const UINT sample_count,
                      synchronized_executor synchronized_transferer, std::function<void(HRESULT)> error_checker)
            : m_device{ device }, m_command_list{ command_list }, m_synchronized_transferer{ std::move(synchronized_transferer) },
              m_error_checker{ std::move(error_checker) } {

            ID3DBlob* root_signature_blob;
            {
                ID3DBlob* error_blob;
                CD3DX12_DESCRIPTOR_RANGE ranges[3];
                ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
                ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
                ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

                CD3DX12_ROOT_PARAMETER parameters[3];
                parameters[0].InitAsDescriptorTable(1, &ranges[0]);
                parameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
                parameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

                const D3D12_ROOT_SIGNATURE_DESC root_signature_desc{
                    std::size(parameters), parameters, 0, nullptr,
                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
                };

                if(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob,
                                               &error_blob) != S_OK) {
                    throw std::runtime_error{ "root signature serialization failed: "s +
                                              static_cast<char*>(error_blob->GetBufferPointer()) };
                }
            }

            check_d3d_error(m_device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
                                                          root_signature_blob->GetBufferSize(),
                                                          IID_PPV_ARGS(m_root_signature.GetAddressOf())));
            root_signature_blob->Release();

            {
                D3D12_DESCRIPTOR_HEAP_DESC desc_heap_for_sampler = {};
                desc_heap_for_sampler.NumDescriptors = 1;
                desc_heap_for_sampler.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
                desc_heap_for_sampler.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                check_d3d_error(m_device->CreateDescriptorHeap(&desc_heap_for_sampler,
                                                               IID_PPV_ARGS(m_sampler_descriptor_heap.GetAddressOf())));

                const D3D12_SAMPLER_DESC sampler_desc{ D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                                                       D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                       D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                       D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                       0.0f,
                                                       0,
                                                       D3D12_COMPARISON_FUNC_ALWAYS,
                                                       {},
                                                       0.0f,
                                                       std::numeric_limits<float>::max() };

                m_device->CreateSampler(&sampler_desc, m_sampler_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
            }

            ID3DBlob* vertex_shader_blob;
            {
                ID3DBlob* compile_log;
                if(D3DCompile(vertex_shader_src.data(), vertex_shader_src.length(), nullptr, nullptr, nullptr, "main", "vs_5_0",
                              D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vertex_shader_blob, &compile_log) != S_OK) {
                    throw std::runtime_error{ "vertex shader compilation failed: "s +
                                              static_cast<char*>(compile_log->GetBufferPointer()) };
                }
            }

            ID3DBlob* pixel_shader_blob;
            {
                ID3DBlob* compile_log;
                if(D3DCompile(pixel_shader_src.data(), pixel_shader_src.length(), nullptr, nullptr, nullptr, "main", "ps_5_0",
                              D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pixel_shader_blob, &compile_log) != S_OK) {
                    throw std::runtime_error{ "pixel shader compilation failed: "s +
                                              static_cast<char*>(compile_log->GetBufferPointer()) };
                }
            }

            const D3D12_INPUT_ELEMENT_DESC input_layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset_u32(&vertex::pos),
                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset_u32(&vertex::tex_coord),
                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset_u32(&vertex::color),
                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            D3D12_BLEND_DESC blend_desc{};
            blend_desc.AlphaToCoverageEnable = blend_desc.IndependentBlendEnable = false;
            blend_desc.RenderTarget[0].BlendEnable = true;
            blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blend_desc.RenderTarget[0].DestBlend = blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            blend_desc.RenderTarget[0].BlendOp = blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            const D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc{
                m_root_signature.Get(),
                { vertex_shader_blob->GetBufferPointer(), vertex_shader_blob->GetBufferSize() },
                { pixel_shader_blob->GetBufferPointer(), pixel_shader_blob->GetBufferSize() },
                {},
                {},
                {},
                {},
                blend_desc,
                0xffffffff,
                { D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, true, 0, 0.0f, 0.0f, false, true, false, 0, {} },
                { false, D3D12_DEPTH_WRITE_MASK_ZERO, D3D12_COMPARISON_FUNC_ALWAYS, false, 0, 0, {}, {} },
                { input_layout, static_cast<UINT>(std::size(input_layout)) },
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                1,
                { DXGI_FORMAT_R8G8B8A8_UNORM },
                DXGI_FORMAT_UNKNOWN,
                { sample_count, 0 },
                0,
                {},
                D3D12_PIPELINE_STATE_FLAG_NONE
            };

            check_d3d_error(
                m_device->CreateGraphicsPipelineState(&pipeline_state_desc, IID_PPV_ARGS(m_pipeline_state.GetAddressOf())));

            vertex_shader_blob->Release();
            pixel_shader_blob->Release();

            const D3D12_DESCRIPTOR_HEAP_DESC heap_desc{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3 + max_textures,
                                                        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
            check_d3d_error(m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(m_descriptor_heap.GetAddressOf())));
            m_cpu_descriptor_handle = m_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
            m_gpu_descriptor_handle = m_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
            m_descriptor_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            constexpr uint64_t constant_buffer_size =
                align(sizeof(constant_buffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

            const auto resource_desc = CD3DX12_RESOURCE_DESC::Buffer(3 * constant_buffer_size);
            const auto heap_properties = CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_UPLOAD };
            check_d3d_error(m_device->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
                                                              D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
                                                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                                              nullptr, IID_PPV_ARGS(m_constant_buffer.GetAddressOf())));

            for(int32_t idx = 0; idx < 3; ++idx) {
                const D3D12_CONSTANT_BUFFER_VIEW_DESC constant_buffer_view_desc{
                    m_constant_buffer->GetGPUVirtualAddress() + idx * constant_buffer_size, constant_buffer_size
                };
                m_device->CreateConstantBufferView(
                    &constant_buffer_view_desc,
                    CD3DX12_CPU_DESCRIPTOR_HANDLE{ m_cpu_descriptor_handle, idx, m_descriptor_increment_size });
            }
            for(uint32_t idx = 0; idx < max_textures; ++idx) {
                m_descriptor_queue.push({ nullptr, 3 + idx });
            }
        }

        explicit d3d12_backend(const d3d12_backend&) = delete;
        explicit d3d12_backend(d3d12_backend&&) = delete;
        d3d12_backend& operator=(const d3d12_backend&) = delete;
        d3d12_backend& operator=(d3d12_backend&&) = delete;

        std::shared_ptr<texture> create_texture(uvec2 size, channel channels) override {
            return std::make_shared<texture_impl>(m_device, m_synchronized_transferer, channels, size, m_error_checker);
        }
        std::shared_ptr<texture> create_texture_from_native_handle(const uint64_t handle, uvec2 size, channel channels) override {
            return std::make_shared<texture_impl>(reinterpret_cast<ID3D12Resource*>(handle), m_device, m_synchronized_transferer,
                                                  channels, size, m_error_checker);
        }
        void update_command_list(const uvec2 window_size, command_queue command_list) override {
            if(m_window_size != window_size) {
                const D3D12_RANGE discard{ 0, 0 };
                std::byte* ptr;
                check_d3d_error(m_constant_buffer->Map(0, &discard, reinterpret_cast<void**>(&ptr)));

                constexpr uint64_t constant_buffer_size =
                    align(sizeof(constant_buffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

                const vec2 size{ static_cast<float>(window_size.x), static_cast<float>(window_size.y) };
                for(int32_t idx = 0; idx < 3; ++idx)
                    *reinterpret_cast<constant_buffer*>(ptr + idx * constant_buffer_size) = { size, idx, 0 };

                const D3D12_RANGE range{ 0, 3 * constant_buffer_size };
                m_constant_buffer->Unmap(0, &range);
            }
            m_window_size = window_size;

            update_vertex_buffer(command_list.vertices);

            m_command_buffer.clear();
            m_command_buffer.reserve(command_list.commands.size());

            for(auto&& command : command_list.commands)
                m_command_buffer.push_back(std::move(command));
        }
        void emit(const uvec2 screen_size) override {
            const auto tp1 = current_time();

            make_dirty();

            uint32_t vertices_offset = 0;

            const vec2 scale = { static_cast<float>(screen_size.x) / m_window_size.x,
                                 static_cast<float>(screen_size.y) / m_window_size.y };

            // ReSharper disable once CppUseStructuredBinding
            for(auto&& command : m_command_buffer) {
                if(command.clip.has_value()) {
                    auto&& clip = command.clip.value();
                    const LONG left = static_cast<int>(std::floor(clip.left * scale.x));
                    const LONG right = static_cast<int>(std::ceil(clip.right * scale.x));
                    const LONG bottom = static_cast<int>(std::ceil(clip.bottom * scale.y));
                    const LONG top = static_cast<int>(std::floor(clip.top * scale.y));
                    const D3D12_RECT clip_rect{ left, top, right, bottom };
                    m_command_list->RSSetScissorRects(1, &clip_rect);
                    m_scissor_restricted = true;
                } else if(m_scissor_restricted) {
                    const D3D12_RECT clip_rect{ 0, 0, static_cast<LONG>(screen_size.x), static_cast<LONG>(screen_size.y) };
                    m_command_list->RSSetScissorRects(1, &clip_rect);
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
            return primitive_type::triangles | primitive_type::triangle_strip;
        }
    };

    ANIMGUI_API std::shared_ptr<render_backend>
    create_d3d12_backend(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, const UINT sample_count,
                         synchronized_executor synchronized_transferer, std::function<void(HRESULT)> error_checker) {
        return std::make_shared<d3d12_backend>(device, command_list, sample_count, std::move(synchronized_transferer),
                                               std::move(error_checker));
    }
}  // namespace animgui
