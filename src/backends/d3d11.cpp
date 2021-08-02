// SPDX-License-Identifier: MIT

#include <animgui/backends/d3d11.hpp>
#include <animgui/core/render_backend.hpp>
#define NOMINMAX
#include "hlsl_shaders.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <utility>

namespace animgui {
    static DXGI_FORMAT get_format(const channel channel) noexcept {
        return channel == channel::alpha ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    class texture_impl final : public texture {
        ID3D11Texture2D* m_texture;
        ID3D11DeviceContext* m_device_context;
        ID3D11ShaderResourceView* m_texture_srv;
        channel m_channel;
        uvec2 m_size;
        bool m_own;
        bool m_dirty;
        const std::function<void(long)>& m_error_checker;

        void check_d3d_error(const HRESULT res) const {
            if(res != S_OK)
                m_error_checker(res);
        }

        void create_texture_srv(ID3D11Device* device) {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            desc.Format = get_format(m_channel);
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = calculate_mipmap_level(m_size);
            desc.Texture2D.MostDetailedMip = 0;
            check_d3d_error(device->CreateShaderResourceView(m_texture, &desc, &m_texture_srv));
        }

    public:
        explicit texture_impl(const texture_impl&) = delete;
        explicit texture_impl(texture_impl&&) = delete;
        texture_impl& operator=(const texture_impl&) = delete;
        texture_impl& operator=(texture_impl&&) = delete;

        texture_impl(ID3D11Device* device, ID3D11DeviceContext* device_context, const channel channel, const uvec2 size,
                     const std::function<void(long)>& error_checker)
            : m_texture{ nullptr }, m_device_context{ device_context }, m_texture_srv{ nullptr }, m_channel{ channel },
              m_size{ size }, m_own{ true }, m_dirty{ false }, m_error_checker{ error_checker } {
            const D3D11_TEXTURE2D_DESC desc{ size.x,
                                             size.y,
                                             calculate_mipmap_level(size),
                                             1,
                                             get_format(channel),
                                             DXGI_SAMPLE_DESC{ 1, 0 },
                                             D3D11_USAGE_DEFAULT,
                                             D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
                                             0,
                                             D3D11_RESOURCE_MISC_GENERATE_MIPS };
            check_d3d_error(device->CreateTexture2D(&desc, nullptr, &m_texture));
            create_texture_srv(device);
        }

        texture_impl(ID3D11Texture2D* handle, ID3D11Device* device, ID3D11DeviceContext* device_context, const channel channel,
                     const uvec2 size, const std::function<void(long)>& error_checker)
            : m_texture{ handle }, m_device_context{ device_context },
              m_texture_srv{ nullptr }, m_channel{ channel }, m_size{ size },
              m_own(false), m_dirty{ false }, m_error_checker{ error_checker } {
            create_texture_srv(device);
        }

        ~texture_impl() override {
            m_texture_srv->Release();
            if(m_own) {
                m_texture->Release();
            }
        }
        void generate_mipmap() override {
            if(m_dirty) {
                m_device_context->GenerateMips(m_texture_srv);
                m_dirty = false;
            }
        }

        void update_texture(const uvec2 offset, const image_desc& image) override {
            if(image.channels != m_channel)
                throw std::runtime_error{ "mismatched channel" };
            if(image.size.x == 0 || image.size.y == 0)
                return;

            auto [size, channels, data] = image;
            std::pmr::vector<uint8_t> rgba;
            if(image.channels == channel::rgb) {
                rgba.resize(static_cast<size_t>(size.x) * size.y * 4);
                auto read_ptr = static_cast<const uint8_t*>(image.data);
                auto write_ptr = rgba.data();
                const auto end = read_ptr + static_cast<size_t>(size.x) * size.y * 3;
                while(read_ptr != end) {
                    *(write_ptr++) = *(read_ptr++);
                    *(write_ptr++) = *(read_ptr++);
                    *(write_ptr++) = *(read_ptr++);
                    *(write_ptr++) = 255;
                }

                data = rgba.data();
            }
            const auto size_dst = m_channel == channel::alpha ? 1 : 4;
            const D3D11_BOX box{ offset.x, offset.y, 0, offset.x + size.x, offset.y + size.y, 1 };
            m_device_context->UpdateSubresource(m_texture, 0, &box, data, size.x * size_dst, 0);
            m_dirty = true;
        }

        [[nodiscard]] uvec2 texture_size() const noexcept override {
            return m_size;
        }

        [[nodiscard]] channel channels() const noexcept override {
            return m_channel;
        }

        [[nodiscard]] uint64_t native_handle() const noexcept override {
            return reinterpret_cast<uint64_t>(m_texture_srv);
        }
    };

    class d3d11_backend final : public render_backend {
        std::pmr::vector<command> m_command_list;
        ID3D11Device* m_device;
        ID3D11DeviceContext* m_device_context;
        std::function<void(long)> m_error_checker;
        vec2 m_window_size;

        ID3D11Buffer* m_vertex_buffer = nullptr;
        size_t m_vertex_buffer_size = 0;
        ID3D11VertexShader* m_vertex_shader = nullptr;
        ID3D11InputLayout* m_input_layout = nullptr;
        ID3D11PixelShader* m_pixel_shader = nullptr;
        ID3D11Buffer* m_constant_buffer = nullptr;
        ID3D11SamplerState* m_sampler_state = nullptr;
        ID3D11RasterizerState* m_rasterizer_state = nullptr;
        ID3D11BlendState* m_blend_state = nullptr;
        ID3D11DepthStencilState* m_depth_stencil_state = nullptr;

        bool m_dirty = false;
        bool m_scissor_restricted = false;
        ID3D11ShaderResourceView* m_bind_tex = nullptr;

        uint64_t m_render_time = 0;

        void check_d3d_error(const HRESULT res) const {
            if(res != S_OK)
                m_error_checker(res);
        }

        void update_vertex_buffer(const std::pmr::vector<vertex>& vertices) {
            if(m_vertex_buffer_size < vertices.size()) {
                if(m_vertex_buffer)
                    m_vertex_buffer->Release();
                m_vertex_buffer_size = std::max(m_vertex_buffer_size, static_cast<size_t>(1024)) * 2;
                const D3D11_BUFFER_DESC vertex_buffer_desc{ static_cast<uint32_t>(m_vertex_buffer_size * sizeof(vertex)),
                                                            D3D11_USAGE_DYNAMIC,
                                                            D3D11_BIND_VERTEX_BUFFER,
                                                            D3D11_CPU_ACCESS_WRITE,
                                                            0,
                                                            sizeof(vertex) };
                check_d3d_error(m_device->CreateBuffer(&vertex_buffer_desc, nullptr, &m_vertex_buffer));
            }

            D3D11_MAPPED_SUBRESOURCE mapped_resource;
            check_d3d_error(m_device_context->Map(m_vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));
            memcpy(mapped_resource.pData, vertices.data(), vertices.size() * sizeof(vertex));
            m_device_context->Unmap(m_vertex_buffer, 0);
        }

        void make_dirty() {
            m_dirty = true;
            m_scissor_restricted = true;
            m_bind_tex = reinterpret_cast<ID3D11ShaderResourceView*>(std::numeric_limits<size_t>::max());
        }

        void emit(const native_callback& callback, uint32_t&) {
            callback();
            make_dirty();
        }
        static D3D11_PRIMITIVE_TOPOLOGY get_primitive_type(const primitive_type type) noexcept {
            // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement CppIncompleteSwitchStatement
            switch(type) {  // NOLINT(clang-diagnostic-switch)
                case primitive_type::triangles:
                    return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
                case primitive_type::triangle_strip:
                    return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            }
            return static_cast<D3D11_PRIMITIVE_TOPOLOGY>(0);
        }

        void emit(const primitives& primitives, uint32_t& vertices_offset) {
            auto&& [type, vertices_count, tex, point_line_size] = primitives;

            if(m_dirty) {
                m_device_context->RSSetState(m_rasterizer_state);

                const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
                m_device_context->OMSetBlendState(m_blend_state, blend_factor, 0xFFFFFFFF);
                m_device_context->OMSetDepthStencilState(m_depth_stencil_state, 0);
                m_device_context->VSSetShader(m_vertex_shader, nullptr, 0);
                m_device_context->VSSetConstantBuffers(0, 1, &m_constant_buffer);
                m_device_context->PSSetShader(m_pixel_shader, nullptr, 0);
                m_device_context->PSSetConstantBuffers(0, 1, &m_constant_buffer);
                m_device_context->IASetInputLayout(m_input_layout);
                m_device_context->GSSetShader(nullptr, nullptr, 0);
                m_device_context->HSSetShader(nullptr, nullptr, 0);
                m_device_context->DSSetShader(nullptr, nullptr, 0);
                m_device_context->CSSetShader(nullptr, nullptr, 0);

                m_device_context->PSSetSamplers(0, 1, &m_sampler_state);

                uint32_t stride = sizeof(vertex);
                uint32_t offset = 0;
                m_device_context->IASetVertexBuffers(0, 1, &m_vertex_buffer, &stride, &offset);

                m_dirty = false;
            }

            {
                // TODO: lazy update
                D3D11_MAPPED_SUBRESOURCE mapped_resource;
                check_d3d_error(m_device_context->Map(m_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));
                auto& uniform = *static_cast<constant_buffer*>(mapped_resource.pData);
                uniform.size = m_window_size;
                uniform.mode = (tex ? (tex->channels() == channel::alpha ? 1 : 0) : 2);
                m_device_context->Unmap(m_constant_buffer, 0);
            }

            m_device_context->IASetPrimitiveTopology(get_primitive_type(type));

            if(tex) {
                const auto texture_srv = reinterpret_cast<ID3D11ShaderResourceView*>(tex->native_handle());
                if(m_bind_tex != texture_srv) {
                    tex->generate_mipmap();
                    m_device_context->PSSetShaderResources(0, 1, &texture_srv);
                    m_bind_tex = texture_srv;
                }
            }

            m_device_context->Draw(vertices_count, vertices_offset);
            vertices_offset += vertices_count;
        }

    public:
        d3d11_backend(ID3D11Device* device, ID3D11DeviceContext* device_context, std::function<void(long)> error_checker)
            : m_device{ device }, m_device_context{ device_context }, m_error_checker{ std::move(error_checker) }, m_window_size{
                  0.0f, 0.0f
              } {

            {
                ID3DBlob* vertex_shader_blob;
                ID3DBlob* compile_log;
                if(D3DCompile(vertex_shader_src.data(), vertex_shader_src.length(), nullptr, nullptr, nullptr, "main", "vs_5_0",
                              D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vertex_shader_blob, &compile_log) != S_OK) {
                    throw std::runtime_error{ "vertex shader compilation failed: "s +
                                              static_cast<char*>(compile_log->GetBufferPointer()) };
                }

                check_d3d_error(m_device->CreateVertexShader(vertex_shader_blob->GetBufferPointer(),
                                                             vertex_shader_blob->GetBufferSize(), nullptr, &m_vertex_shader));

                const D3D11_INPUT_ELEMENT_DESC input_layout[] = {
                    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset_u32(&vertex::pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset_u32(&vertex::tex_coord), D3D11_INPUT_PER_VERTEX_DATA,
                      0 },
                    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset_u32(&vertex::color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                };
                check_d3d_error(m_device->CreateInputLayout(input_layout, 3, vertex_shader_blob->GetBufferPointer(),
                                                            vertex_shader_blob->GetBufferSize(), &m_input_layout));
                vertex_shader_blob->Release();
            }

            {
                ID3DBlob* pixel_shader_blob;
                ID3DBlob* compile_log;
                if(D3DCompile(pixel_shader_src.data(), pixel_shader_src.length(), nullptr, nullptr, nullptr, "main", "ps_5_0",
                              D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pixel_shader_blob, &compile_log) != S_OK) {
                    throw std::runtime_error{ "pixel shader compilation failed: "s +
                                              static_cast<char*>(compile_log->GetBufferPointer()) };
                }
                check_d3d_error(m_device->CreatePixelShader(pixel_shader_blob->GetBufferPointer(),
                                                            pixel_shader_blob->GetBufferSize(), nullptr, &m_pixel_shader));

                pixel_shader_blob->Release();
            }

            {
                const D3D11_BUFFER_DESC constant_buffer_desc{
                    sizeof(constant_buffer), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0,
                    sizeof(constant_buffer)
                };
                check_d3d_error(m_device->CreateBuffer(&constant_buffer_desc, nullptr, &m_constant_buffer));
            }

            {
                D3D11_BLEND_DESC desc{};
                desc.AlphaToCoverageEnable = false;
                desc.RenderTarget[0].BlendEnable = true;
                desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
                desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                desc.RenderTarget[0].BlendOp = desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
                check_d3d_error(m_device->CreateBlendState(&desc, &m_blend_state));
            }

            {
                const D3D11_RASTERIZER_DESC desc{
                    D3D11_FILL_SOLID, D3D11_CULL_NONE, true, 0, 0.0f, 0.0f, false, true, true, true
                };
                check_d3d_error(m_device->CreateRasterizerState(&desc, &m_rasterizer_state));
            }
            {
                D3D11_DEPTH_STENCIL_DESC desc{};
                desc.DepthEnable = desc.StencilEnable = false;
                check_d3d_error(m_device->CreateDepthStencilState(&desc, &m_depth_stencil_state));
            }

            {
                D3D11_SAMPLER_DESC desc{};
                desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
                desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
                desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
                desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
                desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
                desc.MaxAnisotropy = 1;
                desc.MinLOD = 0.0f;
                desc.MaxLOD = std::numeric_limits<float>::max();
                desc.MipLODBias = 0.0f;
                check_d3d_error(m_device->CreateSamplerState(&desc, &m_sampler_state));
            }
        }
        explicit d3d11_backend(const d3d11_backend&) = delete;
        explicit d3d11_backend(d3d11_backend&&) = delete;
        d3d11_backend& operator=(const d3d11_backend&) = delete;
        d3d11_backend& operator=(d3d11_backend&&) = delete;
        ~d3d11_backend() override {
            if(m_vertex_buffer)
                m_vertex_buffer->Release();
            m_vertex_shader->Release();
            m_input_layout->Release();
            m_pixel_shader->Release();
            m_constant_buffer->Release();
            m_sampler_state->Release();
            m_rasterizer_state->Release();
            m_blend_state->Release();
            m_depth_stencil_state->Release();
        }
        void update_command_list(const uvec2 window_size, command_queue command_list) override {
            m_window_size = { static_cast<float>(window_size.x), static_cast<float>(window_size.y) };

            update_vertex_buffer(command_list.vertices);

            m_command_list.clear();
            m_command_list.reserve(command_list.commands.size());

            for(auto&& command : command_list.commands)
                m_command_list.push_back(std::move(command));
        }
        void emit(const uvec2 screen_size) override {
            const auto tp1 = current_time();

            make_dirty();

            uint32_t vertices_offset = 0;

            const vec2 scale = { static_cast<float>(screen_size.x) / m_window_size.x,
                                 static_cast<float>(screen_size.y) / m_window_size.y };

            // ReSharper disable once CppUseStructuredBinding
            for(auto&& command : m_command_list) {
                if(command.clip.has_value()) {
                    auto&& clip = command.clip.value();
                    const LONG left = static_cast<int>(std::floor(clip.left * scale.x));
                    const LONG right = static_cast<int>(std::ceil(clip.right * scale.x));
                    const LONG bottom = static_cast<int>(std::ceil(clip.bottom * scale.y));
                    const LONG top = static_cast<int>(std::floor(clip.top * scale.y));
                    const D3D11_RECT clip_rect{ left, top, right, bottom };
                    m_device_context->RSSetScissorRects(1, &clip_rect);
                    m_scissor_restricted = true;
                } else if(m_scissor_restricted) {
                    const D3D11_RECT clip_rect{ 0, 0, static_cast<LONG>(screen_size.x), static_cast<LONG>(screen_size.y) };
                    m_device_context->RSSetScissorRects(1, &clip_rect);
                    m_scissor_restricted = false;
                }

                std::visit([&](auto&& item) { emit(item, vertices_offset); }, command.desc);
            }

            const auto tp2 = current_time();
            m_render_time = tp2 - tp1;
        }
        std::shared_ptr<texture> create_texture(uvec2 size, channel channels) override {
            return std::make_shared<texture_impl>(m_device, m_device_context, channels, size, m_error_checker);
        }
        std::shared_ptr<texture> create_texture_from_native_handle(const uint64_t handle, uvec2 size, channel channels) override {
            return std::make_shared<texture_impl>(reinterpret_cast<ID3D11Texture2D*>(handle), m_device, m_device_context,
                                                  channels, size, m_error_checker);
        }
        [[nodiscard]] primitive_type supported_primitives() const noexcept override {
            return primitive_type::triangle_strip | primitive_type::triangles;
        }
        [[nodiscard]] uint64_t render_time() const noexcept override {
            return m_render_time;
        }
    };
    ANIMGUI_API std::shared_ptr<render_backend> create_d3d11_backend(ID3D11Device* device, ID3D11DeviceContext* device_context,
                                                                     std::function<void(long)> error_checker) {
        return std::make_shared<d3d11_backend>(device, device_context, std::move(error_checker));
    }
}  // namespace animgui
