指令优化器
===================================

在<animgui/builtins/command_optimizers.hpp>下：

.. code-block:: c++

    // 默认指令优化器，基于连续同类型指令合并以及DAG重建的指令合并算法实现，优化效果好但是性能不佳
    std::shared_ptr<command_optimizer> create_builtin_command_optimizer();

    // 空指令优化器，在显卡性能足够的情况下，不优化更合算
    std::shared_ptr<command_optimizer> create_noop_command_optimizer();
