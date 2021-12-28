渲染后端
===================================

OpenGL3后端
-----------------------------------

基于OpenGL3和glew的后端，最低要求OpenGL3.3。默认全平台构建。

在<animgui/backends/opengl3.hpp>下：

.. code-block:: c++

    std::shared_ptr<render_backend> create_opengl3_backend();

Direct3D 11后端
-----------------------------------

在Windows下默认构建D3D11后端。

在<animgui/backends/d3d11.hpp>下：

.. code-block:: c++

    // device: D3D11的设备实例
    // device_context: D3D11的设备上下文实例
    // error_checker: 渲染后端内部D3D11错误回调，参数类型为HRESULT（long）
    std::shared_ptr<render_backend> create_d3d11_backend(ID3D11Device* device, ID3D11DeviceContext* device_context, const std::function<void(long)>& error_checker);

Direct3D 12后端
-----------------------------------

在Windows下默认构建D3D12后端。

在<animgui/backends/d3d12.hpp>下：

.. code-block:: c++

    using synchronized_executor = std::function<void(const std::function<void(ID3D12GraphicsCommandList*)>&)>;

    // device: D3D12的设备实例
    // command_list: 指令缓冲，用于提交绘制指令
    // synchronized_transferer: 同步指令执行回调，用于纹理阻塞更新。请确保所提交的queue支持transfer
    // sample_count: MSAA采样数
    // error_checker: 渲染后端内部D3D12错误回调，参数类型为HRESULT（long）
    std::shared_ptr<render_backend> create_d3d12_backend(ID3D12Device* device, ID3D12GraphicsCommandList* command_list, uint32_t sample_count,
        synchronized_executor synchronized_transferer, std::function<void(long)> error_checker);


Vulkan后端
-----------------------------------

默认全平台构建Vulkan后端。最低要求Vulkan 1.2。

在<animgui/backends/vulkan.hpp>下：

.. code-block:: c++

    // 同步指令执行回调，目前用于纹理阻塞更新，参考实现参见examples/entrys/vulkan_glfw3.cpp
    using synchronized_executor = std::function<void(const std::function<void(vk::CommandBuffer&)>&)>;


    // physical_device: Vulkan物理设备
    // device: Vulkan逻辑设备
    // render_pass: Vulkan渲染Pass，用于创建渲染流水线
    // command_buffer: 当前指令缓冲，用于提交绘制指令，请在emit前绑定好render_pass
    // sample_count: MSAA采样数
    // sample_shading: 参见 https://www.khronos.org/registry/vulkan/specs/1.2/html/vkspec.html#primsrast-sampleshading
    // synchronized_transfer: 同步指令执行回调，用于纹理阻塞更新。请确保所提交的queue支持transfer
    // error_report: 内部Vulkan错误回调
    ANIMGUI_API std::shared_ptr<render_backend>
    create_vulkan_backend(vk::PhysicalDevice& physical_device, vk::Device& device, vk::RenderPass& render_pass,
                          vk::CommandBuffer& command_buffer, vk::SampleCountFlagBits sample_count, float sample_shading,
                          synchronized_executor synchronized_transfer, std::function<void(vk::Result)> error_report);
                                                                    