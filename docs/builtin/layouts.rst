UI布局
===================================

.. code-block:: c++

    // 逐行布局，所有子组件从左到右，从上到下排列，用newline方法换行
    // parent: 画布，画布管理的绘制区域被当前布局托管
    // alignment: 水平对齐方式，可选left左对齐，right右对齐，middle居中和justify两端对齐
    // render_function: 布局下的UI绘制函数，由用户提供，用户可将传入的row_layout_canvas作为子组件的parent使用，row_layout_canvas的newline方法可以换行
    // 返回值: 布局完毕后所有子组件的包围盒大小
    vec2 layout_row(canvas& parent, const row_alignment alignment, const std::function<void(row_layout_canvas&)>& render_function);

    // 居中布局，在逐行居中布局的基础上垂直居中
    // 参数同逐行布局
    // 返回值：布局完毕后所有子组件的包围盒位置和大小
    bounds_aabb layout_row_center(canvas& parent, const std::function<void(row_layout_canvas&)>& render_function);

    // 单窗口托管，用于提供风格化的窗口标题栏
    // parent: 画布，画布管理的绘制区域被当前布局托管
    // title: 窗口标题
    // attributes: 窗口属性，movable可移动，resizable可调整大小，closable可关闭，minimizable可最小化，no_title_bar无标题栏，maximizable可最大化
    // 注意：单窗口托管下movable和resizable无效，取决于窗口本身的设置
    // render_function: 布局下的UI绘制函数，由用户提供，用户可将传入的window_canvas作为子组件的parent使用，此外close方法可关闭当前窗口，focus方法可使当前窗口处于最顶端
    void single_window(canvas& parent, std::optional<std::pmr::string> title, window_attributes attributes,
                                   const std::function<void(window_canvas&)>& render_function);

    // 内置多窗口管理器，在托管画布内提供虚拟多窗口的功能
    // parent: 画布，画布管理的绘制区域被当前布局托管
    // render_function: 布局下的UI绘制函数
    void multiple_window(canvas& parent, const std::function<void(multiple_window_canvas&)>& render_function);

    class multiple_window_canvas : public layout_proxy {
    public:
        explicit multiple_window_canvas(canvas& parent) noexcept : layout_proxy{ parent } {}
        // 参见单窗口托管
        // id: 窗口标识符，用于区分窗口，推荐使用mix或_id生成，也可以自行设置
        // 注意：不支持minimizable和maximizable，暂不支持resizable
        // 注意：当前实现下，当窗口第一次被声明时，窗口默认打开并置于最顶端
        virtual void new_window(identifier id, std::optional<std::pmr::string> title, window_attributes attributes,
                                const std::function<void(window_canvas&)>& render_function) = 0;
        // 根据id关闭窗口
        virtual void close_window(identifier id) = 0;
        // 根据id打开被关闭的窗口
        virtual void open_window(identifier id) = 0;
        // 根据id将对应窗口置于最顶端
        virtual void focus_window(identifier id) = 0;
    };
