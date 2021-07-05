纹理分配器
===================================

在<animgui/builtins/image_compactors.hpp>下：

.. code-block:: c++

    // 默认基于链表的纹理分配器
    // render_backend: 渲染后端，提供纹理的分配和消除
    // memory_resource: pmr多态内存分配器
    std::shared_ptr<image_compactor> create_builtin_image_compactor(render_backend& render_backend,
        std::pmr::memory_resource* memory_resource);
