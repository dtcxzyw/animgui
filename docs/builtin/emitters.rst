指令发射器
===================================

在<animgui/builtins/emitters.hpp>下：

.. code-block:: c++

    // 默认指令发射器
    // memory_resource: pmr多态内存分配器，用于临时状态的内存分配
    std::shared_ptr<emitter> create_builtin_emitter(std::pmr::memory_resource* memory_resource);
