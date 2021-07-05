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
