// SPDX-License-Identifier: MIT

#define NOMINMAX
#include "../application.hpp"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include "animgui/core/input_backend.hpp"

#include <GLFW/glfw3native.h>
#include <animgui/backends/d3d11.hpp>
#include <animgui/backends/glfw3.hpp>
#include <animgui/backends/stbfont.hpp>
#include <animgui/builtins/animators.hpp>
#include <animgui/builtins/command_optimizers.hpp>
#include <animgui/builtins/emitters.hpp>
#include <animgui/builtins/image_compactors.hpp>
#include <animgui/builtins/layouts.hpp>
#include <animgui/core/context.hpp>
#include <d3d11.h>
#include <iostream>
#include <string>

[[noreturn]] void fail(const std::string& str) {
    std::cout << str << std::endl;
    std::terminate();
}

void check_d3d11_error(const HRESULT res) {
    if(res != S_OK)
        fail("D3D11 Error " + std::to_string(res));
}

int main() {
    glfwSetErrorCallback([](const int id, const char* error) { fail("[GLFW:" + std::to_string(id) + "] " + error); });
    if(!glfwInit())
        fail("Failed to initialize glfw");

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    int w = 1024, h = 768;
    GLFWwindow* const window = glfwCreateWindow(w, h, "Animgui demo ( d3d11_glfw3 )", nullptr, nullptr);
    int screen_w, screen_h;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), nullptr, nullptr, &screen_w, &screen_h);
    glfwSetWindowPos(window, (screen_w - w) / 2, (screen_h - h) / 2);

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{ { static_cast<uint32_t>(w),
                                            static_cast<uint32_t>(h),
                                            {},
                                            DXGI_FORMAT_R8G8B8A8_UNORM,
                                            DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                                            DXGI_MODE_SCALING_UNSPECIFIED },
                                          { 8, 0 },
                                          DXGI_USAGE_RENDER_TARGET_OUTPUT,
                                          2,
                                          glfwGetWin32Window(window),
                                          true,
                                          DXGI_SWAP_EFFECT_DISCARD,
                                          0 };
    IDXGISwapChain* swap_chain;
    ID3D11Device* device;
    ID3D11DeviceContext* device_context;
    check_d3d11_error(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG, levels,
        std::size(levels), D3D11_SDK_VERSION, &swap_chain_desc, &swap_chain, &device, nullptr, &device_context));

    device->SetExceptionMode(D3D11_RAISE_FLAG_DRIVER_INTERNAL_ERROR);

    ID3D11RenderTargetView* render_target_view;

    auto create_render_target_view = [&] {
        ID3D11Texture2D* back_buffer;
        check_d3d11_error(swap_chain->GetBuffer(0,
                                                __uuidof(ID3D11Texture2D),  // NOLINT(clang-diagnostic-language-extension-token)
                                                reinterpret_cast<void**>(&back_buffer)));
        check_d3d11_error(device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view));
        device_context->OMSetRenderTargets(1, &render_target_view, nullptr);
        back_buffer->Release();
    };

    create_render_target_view();

    {
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();
        std::function<void()> draw;
        const auto glfw3_backend = animgui::create_glfw3_backend(window, draw);
        const auto d3d11_backend = animgui::create_d3d11_backend(device, device_context, check_d3d11_error);
        const auto stb_font_backend = animgui::create_stb_font_backend(8.0f);
        const auto animator = animgui::create_dummy_animator();
        const auto emitter = animgui::create_builtin_emitter(memory_resource);
        const auto command_optimizer = animgui::create_builtin_command_optimizer();
        const auto image_compactor = animgui::create_builtin_image_compactor(*d3d11_backend, memory_resource);
        auto ctx = animgui::create_animgui_context(*glfw3_backend, *d3d11_backend, *stb_font_backend, *emitter, *animator,
                                                   *command_optimizer, *image_compactor, memory_resource);
        const auto app = animgui::create_demo_application(*ctx);

        auto last = glfwGetTime();

        draw = [&] {
            int cur_w, cur_h;
            glfwGetFramebufferSize(window, &cur_w, &cur_h);

            if(cur_w == 0 || cur_h == 0)
                return;

            if(cur_w != w || cur_h != h) {
                render_target_view->Release();
                check_d3d11_error(swap_chain->ResizeBuffers(0, cur_w, cur_h, DXGI_FORMAT_UNKNOWN, 0));
                create_render_target_view();

                w = cur_w;
                h = cur_h;
            }

            const auto current = glfwGetTime();
            const auto delta_t = static_cast<float>(current - last);
            last = current;

            int window_w, window_h;
            glfwGetWindowSize(window, &window_w, &window_h);

            ctx->new_frame(window_w, window_h, delta_t, [&](animgui::canvas& canvas_root) { app->render(canvas_root); });

            D3D11_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 0.0f };
            device_context->RSSetViewports(1, &viewport);
            const D3D11_RECT clip_rect{ 0, 0, w, h };
            device_context->RSSetScissorRects(1, &clip_rect);
            float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            device_context->ClearRenderTargetView(render_target_view, clear_color);

            d3d11_backend->emit(animgui::uvec2{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });

            check_d3d11_error(swap_chain->Present(0, 0));
        };

        while(!glfwWindowShouldClose(window)) {
            glfw3_backend->new_frame();
            draw();
        }
    }

    swap_chain->Release();
    render_target_view->Release();
    device->Release();
    device_context->Release();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
