窗口管理器
===================================

glfw3后端
-----------------------------------

基于glfw3的后端。默认全平台构建。

在<animgui/backends/glfw3.hpp>下：

.. code-block:: c++

    // window: 用户自行创建的glfw window上下文，注意后端会覆盖注册输入事件回调
    // redraw: 重绘函数，当窗口大小改变时被调用，主要用于提升窗口大小改变过程中的流畅性
    std::shared_ptr<input_backend> create_glfw3_backend(GLFWwindow* window, const std::function<void()>& redraw)
