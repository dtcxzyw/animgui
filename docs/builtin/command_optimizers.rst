指令优化器
===================================

指令优化器暂未实现。暂时使用create_builtin_command_optimizer()代替。

在<animgui/builtins/command_optimizers.hpp>下：

.. code-block:: c++

    // 默认指令优化器
    std::shared_ptr<command_optimizer> create_builtin_command_optimizer();
