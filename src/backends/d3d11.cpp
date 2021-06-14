// SPDX-License-Identifier: MIT

#include <animgui/backends/d3d11.hpp>
#include <animgui/core/render_backend.hpp>
#include <string_view>
#define NOMINMAX
#include <cassert>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <utility>

using namespace std::literals;

namespace animgui {
    static DXGI_FORMAT get_format(const channel channel) noexcept {
        if(channel == channel::alpha)
            return DXGI_FORMAT_R8_UNORM;
        if(channel == channel::rgb)
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    class texture_impl final : public texture {
        ID3D11Texture2D* m_texture;
        ID3D11DeviceContext* m_device_context;
        channel m_channel;
        uvec2 m_size;
        bool m_own;
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

        texture_impl(ID3D11Device* device, ID3D11DeviceContext* device_context, const channel channel, const uvec2 size,
                     const std::function<void(long)>& error_checker)
            : m_texture{ nullptr }, m_device_context{ device_context }, m_channel{ channel }, m_size{ size }, m_own{ true },
              m_error_checker{ error_checker } {
            D3D11_TEXTURE2D_DESC desc{ size.x,
                                       size.y,
                                       1,
                                       1,
                                       get_format(channel),
                                       DXGI_SAMPLE_DESC{ 1, 0 },
                                       D3D11_USAGE_DEFAULT,
                                       D3D11_BIND_SHADER_RESOURCE,
                                       0,
                                       0 };
            check_d3d_error(device->CreateTexture2D(&desc, nullptr, &m_texture));
        }

        texture_impl(ID3D11Texture2D* handle, ID3D11DeviceContext* device_context, const channel channel, const uvec2 size,
                     const std::function<void(long)>& error_checker)
            : m_texture{ handle }, m_device_context{ device_context }, m_channel{ channel }, m_size{ size },
              m_own(false), m_error_checker{ error_checker } {}

        ~texture_impl() override {
            if(m_own) {
                m_texture->Release();
            }
        }

        void update_texture(const uvec2 offset, const image_desc& image) override {
            if(image.channels != m_channel)
                throw std::runtime_error{ "mismatched channel" };

            image_desc src = image;
            std::pmr::vector<uint8_t> rgba;
            if(image.channels == channel::rgb) {
                rgba.resize(image.size.x * image.size.y * 4);
                auto read_ptr = static_cast<const uint8_t*>(image.data);
                auto write_ptr = rgba.data();
                const auto end = read_ptr + image.size.x * image.size.y * 3;
                while(read_ptr != end) {
                    *(write_ptr++) = *(read_ptr++);
                    *(write_ptr++) = *(read_ptr++);
                    *(write_ptr++) = *(read_ptr++);
                    *(write_ptr++) = 255;
                }

                src.data = rgba.data();
            }
            const auto size_dst = m_channel == channel::alpha ? 1 : 4;
            D3D11_BOX box{ offset.x, offset.y, 0, offset.x + src.size.x, offset.y + src.size.y, 1 };
            m_device_context->UpdateSubresource(m_texture, 0, &box, src.data, src.size.x * size_dst, 0);
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

    static auto vertex_shader_src = R"(
        cbuffer constant_buffer : register(b0) {
            float2 size;
            int mode;
            int padding;
        };
        struct VS_INPUT {
            float2 pos : POSITION;
            float2 tex_coord  : TEXCOORD0;
            float4 color : COLOR0;
        };
            
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex_coord  : TEXCOORD0;
            float4 color : COLOR0;
        };

        PS_INPUT main(VS_INPUT input) {
            PS_INPUT output;
            output.pos = float4(input.pos.x/size.x*2.0f-1.0f,1.0f-input.pos.y/size.y*2.0f, 0.0f, 1.0f);
            output.tex_coord  = input.tex_coord;
            output.color = input.color;
            return output;
        }
    )"sv;

    static auto pixel_shader_src = R"(
        cbuffer constant_buffer : register(b0) {
            float2 size;
            int mode;
            int padding;
        };
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex_coord  : TEXCOORD0;
            float4 color : COLOR0;
        };
        sampler sampler0;
        Texture2D texture0;
        
        float4 main(PS_INPUT input) : SV_Target {
            if(mode==0)
                return input.color * texture0.Sample(sampler0, input.tex_coord);
            if(mode==1)
                return input.color * texture0.Sample(sampler0, input.tex_coord).xxxx;
            return input.color;
        }
    )"sv;

    struct constant_buffer final {
        vec2 size;
        int mode;
        int padding;
    };

    static_assert(sizeof(constant_buffer) % 16 == 0);

    class d3d11_backend final : public render_backend {
        std::pmr::vector<command> m_command_list;
        ID3D11Device* m_device;
        ID3D11DeviceContext* m_device_context;
        std::function<void(long)> m_error_checker;

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

        std::pmr::unordered_map<uint64_t, ID3D11ShaderResourceView*> m_texture_shader_resource_view;

        void check_d3d_error(const HRESULT res) const {
            if(res != S_OK)
                m_error_checker(res);
        }

        void update_vertex_buffer(const std::pmr::vector<vertex>& vertices) {
            if(m_vertex_buffer_size < vertices.size()) {
                if(m_vertex_buffer)
                    m_vertex_buffer->Release();
                m_vertex_buffer_size = std::max(m_vertex_buffer_size, 1024ULL) * 2;
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

        static void emit(const native_callback& callback, const bounds& clip, uvec2) {
            callback(clip);
        }
        static D3D11_PRIMITIVE_TOPOLOGY get_primitive_type(const primitive_type type) noexcept {
            // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
            // ReSharper disable once CppIncompleteSwitchStatement
            switch(type) {
                case primitive_type::triangles:
                    return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
                case primitive_type::triangle_strip:
                    return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            }
            return static_cast<D3D11_PRIMITIVE_TOPOLOGY>(0);
        }

        ID3D11ShaderResourceView* get_texture_view(const uint64_t texture, const channel channels) {
            auto&& view = m_texture_shader_resource_view[texture];
            if(!view) {
                D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = get_format(channels);
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipLevels = 1;
                desc.Texture2D.MostDetailedMip = 0;
                check_d3d_error(m_device->CreateShaderResourceView(reinterpret_cast<ID3D11Resource*>(texture), &desc, &view));
            }
            return view;
        }

        void emit(const primitives& primitives, const bounds& clip, const uvec2 screen_size) {
            auto&& [type, vertices, texture, point_line_size] = primitives;

            m_device_context->RSSetState(m_rasterizer_state);
            const LONG left = static_cast<int>(std::floor(clip.left));
            const LONG right = static_cast<int>(std::ceil(clip.right));
            const LONG bottom = static_cast<int>(std::ceil(clip.bottom));
            const LONG top = static_cast<int>(std::floor(clip.top));
            const D3D11_RECT clip_rect{ left, top, right, bottom };
            m_device_context->RSSetScissorRects(1, &clip_rect);

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
            update_vertex_buffer(primitives.vertices);
            uint32_t stride = sizeof(vertex);
            uint32_t offset = 0;
            m_device_context->IASetVertexBuffers(0, 1, &m_vertex_buffer, &stride, &offset);

            {
                // TODO: lazy update
                D3D11_MAPPED_SUBRESOURCE mapped_resource;
                check_d3d_error(m_device_context->Map(m_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));
                auto& uniform = *static_cast<constant_buffer*>(mapped_resource.pData);
                uniform.size = { static_cast<float>(screen_size.x), static_cast<float>(screen_size.y) };
                uniform.mode = (texture ? (texture->channels() == channel::alpha ? 1 : 0) : 2);
                m_device_context->Unmap(m_constant_buffer, 0);
            }
            m_device_context->IASetPrimitiveTopology(get_primitive_type(type));
            if(texture) {
                m_device_context->PSSetSamplers(0, 1, &m_sampler_state);
                const auto texture_srv = get_texture_view(texture->native_handle(), texture->channels());
                m_device_context->PSSetShaderResources(0, 1, &texture_srv);
            }

            m_device_context->Draw(static_cast<uint32_t>(vertices.size()), 0);
        }

    public:
        d3d11_backend(ID3D11Device* device, ID3D11DeviceContext* device_context, std::function<void(long)> error_checker)
            : m_device{ device }, m_device_context{ device_context }, m_error_checker{ std::move(error_checker) } {

            {
                ID3DBlob* vertex_shader_blob;
                ID3DBlob* compile_log;
                if(D3DCompile(vertex_shader_src.data(), vertex_shader_src.length(), nullptr, nullptr, nullptr, "main", "vs_5_0",
                              0, 0, &vertex_shader_blob, &compile_log) != S_OK) {
                    throw std::runtime_error{ "vertex shader compilation failed: "s +
                                              static_cast<char*>(compile_log->GetBufferPointer()) };
                }

                check_d3d_error(m_device->CreateVertexShader(vertex_shader_blob->GetBufferPointer(),
                                                             vertex_shader_blob->GetBufferSize(), nullptr, &m_vertex_shader));

                D3D11_INPUT_ELEMENT_DESC input_layout[] = {
                    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                      static_cast<uint32_t>(reinterpret_cast<uint64_t>(offset(&vertex::pos))), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                      static_cast<uint32_t>(reinterpret_cast<uint64_t>(offset(&vertex::tex_coord))), D3D11_INPUT_PER_VERTEX_DATA,
                      0 },
                    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                      static_cast<uint32_t>(reinterpret_cast<uint64_t>(offset(&vertex::color))), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                };
                check_d3d_error(m_device->CreateInputLayout(input_layout, 3, vertex_shader_blob->GetBufferPointer(),
                                                            vertex_shader_blob->GetBufferSize(), &m_input_layout));
                vertex_shader_blob->Release();
            }

            {
                ID3DBlob* pixel_shader_blob;
                ID3DBlob* compile_log;
                if(D3DCompile(pixel_shader_src.data(), pixel_shader_src.length(), nullptr, nullptr, nullptr, "main", "ps_5_0", 0,
                              0, &pixel_shader_blob, &compile_log) != S_OK) {
                    throw std::runtime_error{ "pixel shader compilation failed: "s +
                                              static_cast<char*>(compile_log->GetBufferPointer()) };
                }
                check_d3d_error(m_device->CreatePixelShader(pixel_shader_blob->GetBufferPointer(),
                                                            pixel_shader_blob->GetBufferSize(), nullptr, &m_pixel_shader));

                pixel_shader_blob->Release();
            }

            {
                D3D11_BUFFER_DESC constant_buffer_desc{
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
                D3D11_RASTERIZER_DESC desc{ D3D11_FILL_SOLID, D3D11_CULL_NONE, true, 0, 0.0f, 0.0f, false, true, true, true };
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
                desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
                desc.MaxAnisotropy = 0;
                desc.MinLOD = desc.MaxLOD = 0.0f;
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
            for(auto&& [key, view] : m_texture_shader_resource_view)
                view->Release();
        }
        void update_command_list(std::pmr::vector<command> command_list) override {
            m_command_list = std::move(command_list);
        }
        void emit(uvec2 screen_size) override {
            // ReSharper disable once CppUseStructuredBinding
            for(auto&& command : m_command_list) {
                auto&& clip = command.clip;
                std::visit([&](auto&& item) { emit(item, clip, screen_size); }, command.desc);
            }
        }
        std::shared_ptr<texture> create_texture(uvec2 size, channel channels) override {
            return std::make_shared<texture_impl>(m_device, m_device_context, channels, size, m_error_checker);
        }
        std::shared_ptr<texture> create_texture_from_native_handle(const uint64_t handle, uvec2 size, channel channels) override {
            return std::make_shared<texture_impl>(reinterpret_cast<ID3D11Texture2D*>(handle), m_device_context, channels, size,
                                                  m_error_checker);
        }
        [[nodiscard]] primitive_type supported_primitives() const noexcept override {
            return primitive_type::triangle_strip | primitive_type::triangles;
        }
    };
    ANIMGUI_API std::shared_ptr<render_backend> create_d3d11_backend(ID3D11Device* device, ID3D11DeviceContext* device_context,
                                                                     const std::function<void(long)>& error_checker) {
        return std::make_shared<d3d11_backend>(device, device_context, error_checker);
    }
}  // namespace animgui
