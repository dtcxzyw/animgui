UI组件
===================================

.. code-block:: c++

    // 标签
    // parent: 画布
    // str: 文本
    void text(canvas& parent, std::pmr::string str);

    // 图片
    // parent: 画布
    // image: 图片，通过context.load_image获得
    // size: 图片大小
    // factor: 颜色系数，整张图片的颜色乘上系数后显示，主要用于实现透明效果
    void image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor);
    
    // 普通按钮
    // parent: 画布
    // label: 按钮标签
    // 返回值: 按钮被按下一次则返回true，否则false
    bool button_label(canvas& parent, std::pmr::string label);
    
    // 图片按钮
    // parent: 画布
    // image: 图片，通过context.load_image获得
    // size: 图片大小
    // factor: 颜色系数，整张图片的颜色乘上系数后显示，主要用于实现透明效果
    // 返回值: 按钮被按下一次则返回true，否则false
    bool button_image(canvas& parent, texture_region image, vec2 size, const color_rgba& factor);
    
    // 整数滑动条
    // parent: 画布
    // width: 滑动条宽度
    // handle_width: 滑动条把手宽度
    // val: 当前值
    // min: 最小值
    // max: 最大值
    void slider(canvas& parent, float width, float handle_width, int32_t& val, int32_t min, int32_t max);
    
    // 浮点滑动条
    // parent: 画布
    // width: 滑动条宽度
    // handle_width: 滑动条把手宽度
    // val: 当前值
    // min: 最小值
    // max: 最大值
    void slider(canvas& parent, float width, float handle_width, float& val, float min, float max);
    
    // 复选框
    // parent: 画布
    // label: 标签
    // state: 当前状态
    void checkbox(canvas& parent, std::pmr::string label, bool& state);
    
    // 开关
    // parent: 画布
    // state: 当前状态
    void switch_(canvas& parent, bool& state);
    
    // 单行文本编辑框
    // parent: 画布
    // glyph_width: 希望完全显示的字符数（估计）
    // str: 当前文本
    // placeholder: 文本编辑框为空时显示的内容
    void text_edit(canvas& parent, float glyph_width, std::pmr::string& str, std::optional<std::pmr::string> placeholder = std::nullopt);
    
    // 单选按钮
    // parent: 画布
    // labels: 所有标签
    // index: 当前选中项，index无效时为未选中任何项
    void radio_button(canvas& parent, const std::pmr::vector<std::pmr::string>& labels, size_t& index);
    
    // 进度条
    // parent: 画布
    // width: 进度条宽度
    // progress: 当前进度
    // label: 标签
    void progressbar(canvas& parent, float width, float progress, std::optional<std::pmr::string> label);
