内置主题
===================================

通用风格设计暂未完成。暂时使用set_classic_style()代替。

在<animgui/builtins/styles.hpp>下：

.. code-block:: c++

    // 默认风格
    // context: 上下文
    void set_classic_style(context& context);

注意：style.default_font需要用户自行通过context.load_font方法加载。
