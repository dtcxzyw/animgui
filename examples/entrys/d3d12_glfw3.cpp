// SPDX-License-Identifier: MIT

#define NOMINMAX
#include "../application.hpp"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <animgui/backends/d3d12.hpp>
#include <animgui/backends/glfw3.hpp>
#include <animgui/backends/stbfont.hpp>
#include <animgui/builtins/animators.hpp>
#include <animgui/builtins/command_optimizers.hpp>
#include <animgui/builtins/emitters.hpp>
#include <animgui/builtins/image_compactors.hpp>
#include <animgui/builtins/layouts.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/input_backend.hpp>
#include <array>
#include <cstdlib>
#include <directx/d3dx12.h>
#include <dxgi1_4.h>
#include <iostream>
#include <string>

[[noreturn]] void fail(const std::string& str) {
    std::cout << str << std::endl;
    std::_Exit(EXIT_FAILURE);
}

void check_d3d12_error(const HRESULT res) {
    if(res != S_OK)
        fail("D3D12 Error " + std::to_string(res));
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
    GLFWwindow* const window = glfwCreateWindow(w, h, "Animgui demo (d3d12_glfw3)", nullptr, nullptr);
    int screen_w, screen_h;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), nullptr, nullptr, &screen_w, &screen_h);
    glfwSetWindowPos(window, (screen_w - w) / 2, (screen_h - h) / 2);

    IDXGIFactory4* dxgi_factory;
    check_d3d12_error(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));

    IDXGIAdapter1* dxgi_adapter = nullptr;
    for(UINT idx = 0; dxgi_factory->EnumAdapters1(idx, &dxgi_adapter) != DXGI_ERROR_NOT_FOUND; ++idx) {
        DXGI_ADAPTER_DESC1 desc;
        check_d3d12_error(dxgi_adapter->GetDesc1(&desc));
        if(!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            std::cout << "D3D12 Adapter: " << desc.Description << std::endl;
            break;
        }
        dxgi_adapter->Release();
    }

    ID3D12Device* device;
    check_d3d12_error(D3D12CreateDevice(dxgi_adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)));
    // TODO: debug layer

    ID3D12CommandQueue* command_queue;
    const D3D12_COMMAND_QUEUE_DESC command_queue_desc{};
    check_d3d12_error(device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue)));

    constexpr UINT buffer_count = 2;
    constexpr UINT sample_count = 8;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc{ { static_cast<uint32_t>(w),
                                            static_cast<uint32_t>(h),
                                            {},
                                            DXGI_FORMAT_R8G8B8A8_UNORM,
                                            DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
                                            DXGI_MODE_SCALING_UNSPECIFIED },
                                          { sample_count, 0 },
                                          DXGI_USAGE_RENDER_TARGET_OUTPUT,
                                          buffer_count,
                                          glfwGetWin32Window(window),
                                          true,
                                          DXGI_SWAP_EFFECT_FLIP_DISCARD,
                                          0 };
    IDXGISwapChain3* swap_chain;
    check_d3d12_error(dxgi_factory->CreateSwapChain(device, &swap_chain_desc, reinterpret_cast<IDXGISwapChain**>(&swap_chain)));

    const D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, buffer_count,
                                                           D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    ID3D12DescriptorHeap* descriptor_heap;
    check_d3d12_error(device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap)));
    std::array<std::tuple<ID3D12Resource*, ID3D12CommandAllocator*, ID3D12Fence*, uint64_t>, buffer_count> frame_buffers;

    const auto rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle{ descriptor_heap->GetCPUDescriptorHandleForHeapStart() };

        for(uint32_t idx = 0; idx < buffer_count; ++idx) {
            auto&& [render_target_view, command_allocator, fence, fence_count] = frame_buffers[idx];
            check_d3d12_error(swap_chain->GetBuffer(idx, IID_PPV_ARGS(&render_target_view)));
            device->CreateRenderTargetView(render_target_view, nullptr, rtv_handle);
            rtv_handle.Offset(1, rtv_descriptor_size);
            check_d3d12_error(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)));
            check_d3d12_error(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        }
    }

    const auto fence_event = CreateEventA(nullptr, false, false, nullptr);
    const auto synchronized_fence_event = CreateEventA(nullptr, false, false, nullptr);
    ID3D12Fence* synchronized_fence;
    check_d3d12_error(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&synchronized_fence)));

    ID3D12GraphicsCommandList* command_list;
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, std::get<ID3D12CommandAllocator*>(frame_buffers[0]), nullptr,
                              IID_PPV_ARGS(&command_list));

    ID3D12CommandAllocator* synchronized_allocator;
    check_d3d12_error(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&synchronized_allocator)));

    {
        const auto synchronized_transfer = [&](const std::function<void(ID3D12GraphicsCommandList*)>& callback) {
            check_d3d12_error(synchronized_allocator->Reset());
            check_d3d12_error(command_list->Reset(synchronized_allocator, nullptr));
            callback(command_list);
            check_d3d12_error(command_list->Close());
            command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&command_list));
            const auto next_value = synchronized_fence->GetCompletedValue();
            synchronized_fence->SetEventOnCompletion(next_value + 1, synchronized_fence_event);
            check_d3d12_error(command_queue->Signal(synchronized_fence, next_value));
            WaitForSingleObject(synchronized_fence_event, INFINITE);
        };
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();
        std::function<void()> draw;
        const auto glfw3_backend = animgui::create_glfw3_backend(window, draw);
        const auto d3d12_backend =
            animgui::create_d3d12_backend(device, command_list, sample_count, synchronized_transfer, check_d3d12_error);
        const auto stb_font_backend = animgui::create_stb_font_backend(8.0f);
        const auto animator = animgui::create_dummy_animator();
        const auto emitter = animgui::create_builtin_emitter(memory_resource);
        const auto command_optimizer = animgui::create_builtin_command_optimizer();
        const auto image_compactor = animgui::create_builtin_image_compactor(*d3d12_backend, memory_resource);
        auto ctx = animgui::create_animgui_context(*glfw3_backend, *d3d12_backend, *stb_font_backend, *emitter, *animator,
                                                   *command_optimizer, *image_compactor, memory_resource);
        const auto app = animgui::create_demo_application(*ctx);

        auto last = glfwGetTime();

        draw = [&] {
            int cur_w, cur_h;
            glfwGetFramebufferSize(window, &cur_w, &cur_h);

            if(cur_w == 0 || cur_h == 0)
                return;

            if(cur_w != w || cur_h != h) {

                check_d3d12_error(swap_chain->ResizeBuffers(0, cur_w, cur_h, DXGI_FORMAT_UNKNOWN, 0));

                w = cur_w;
                h = cur_h;
            }

            const auto current = glfwGetTime();
            const auto delta_t = static_cast<float>(current - last);
            last = current;

            int window_w, window_h;
            glfwGetWindowSize(window, &window_w, &window_h);

            ctx->new_frame(window_w, window_h, delta_t, [&](animgui::canvas& canvas_root) { app->render(canvas_root); });

            const auto frame_idx = swap_chain->GetCurrentBackBufferIndex();
            auto& [render_target_view, command_allocator, fence, fence_count] = frame_buffers[frame_idx];

            if(fence->GetCompletedValue() < fence_count) {
                check_d3d12_error(fence->SetEventOnCompletion(fence_count, fence_event));
                WaitForSingleObject(fence_event, INFINITE);
            }
            ++fence_count;

            check_d3d12_error(command_allocator->Reset());
            check_d3d12_error(command_list->Reset(command_allocator, nullptr));

            const auto barrier_enter = CD3DX12_RESOURCE_BARRIER::Transition(render_target_view, D3D12_RESOURCE_STATE_PRESENT,
                                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
            command_list->ResourceBarrier(1, &barrier_enter);

            const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle{ descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                                                            static_cast<int32_t>(frame_idx), rtv_descriptor_size };

            D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f };
            command_list->RSSetViewports(1, &viewport);
            const D3D12_RECT clip_rect{ 0, 0, w, h };
            command_list->RSSetScissorRects(1, &clip_rect);

            command_list->OMSetRenderTargets(1, &rtv_handle, false, nullptr);

            float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);

            d3d12_backend->emit(animgui::uvec2{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });

            const auto barrier_exit = CD3DX12_RESOURCE_BARRIER::Transition(render_target_view, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                           D3D12_RESOURCE_STATE_PRESENT);
            command_list->ResourceBarrier(1, &barrier_exit);

            check_d3d12_error(command_list->Close());

            command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&command_list));
            check_d3d12_error(command_queue->Signal(fence, fence_count));

            check_d3d12_error(swap_chain->Present(0, 0));
        };

        while(!glfwWindowShouldClose(window)) {
            glfw3_backend->new_frame();
            draw();
        }
    }

    synchronized_allocator->Release();
    command_list->Release();
    synchronized_fence->Release();

    for(auto&& [frame_buffer, allocator, fence, _] : frame_buffers) {
        frame_buffer->Release();
        allocator->Release();
        fence->Release();
    }

    descriptor_heap->Release();
    swap_chain->Release();
    command_queue->Release();
    device->Release();
    dxgi_adapter->Release();
    dxgi_factory->Release();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
