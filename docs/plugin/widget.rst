扩展UI组件
===================================

扩展UI组件的方法与扩展UI布局类似，都是利用canvas来实现功能。UI组件与UI布局的区别是UI组件不提供自定义回调。
请参考扩展UI布局的文档。

常用帮助函数（处理一些极端情况）

.. code-block:: c++

    // 判断当前组件是否被选中，主要用于可拖动组件
    // parent: 画布
    // id: 唯一标识符
    // 返回值：只有在当前区域中按下不松开，才返回true
    bool selected(canvas& parent, identifier id);
    
    // 判断当前组件是否被点击，主要用于类按钮组件
    // parent: 画布
    // id: 唯一标识符
    // pressed: 鼠标或游戏手柄是否按下
    // focused: 当前区域是否拥有焦点
    // 返回值：只有在当前区域中按下并弹起，才返回true
    bool clicked(canvas& parent, identifier id, bool pressed, bool focused);

注意事项
- 由于组件是匿名的，组件的根区域必须以region_sub_uid作为标识符，因此暂时不支持动态隐藏组件

具体示例参见已有UI组件的实现 (animgui/builtins/widgets.cpp)
