字体后端
===================================

stb_font后端
-----------------------------------

基于stb_truetype的后端，暂时只支持光栅化。默认全平台构建。

在<animgui/backends/stbfont.hpp>下：

.. code-block:: c++

    // super_sample: 超采样倍数，降低文字光栅化渲染的锯齿感
    std::shared_ptr<font_backend> create_stb_font_backend(float super_sample = 1.0f);
