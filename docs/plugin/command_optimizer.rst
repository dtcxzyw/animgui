扩展指令优化器
===================================

参见<animgui/core/command_optimizer.hpp>:

扩展指令发射器需要派生自command_optimizer并实现以下接口：

.. code-block:: c++

    class command_optimizer {
    public:
        // 对渲染指令进行优化
        // size: 整个窗口的大小，用于辅助分块
        // src: 原渲染指令
        // 返回值: 返回优化后的渲染指令序列，注意不要使用src中不存在的图元
        [[nodiscard]] virtual command_queue optimize(uvec2 size, command_queue src) const = 0;
        // 返回指令优化器支持的图元类型
        [[nodiscard]] virtual primitive_type supported_primitives() const noexcept = 0;
    };


具体示例可参考builtins/command_optimizers.cpp。
