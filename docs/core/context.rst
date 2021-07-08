核心上下文
===================================

.. code-block:: c++

    class context {
    public:
        // 渲染新的一帧
        // width/height: 窗口大小，注意不是帧缓冲区大小
        // delta_t: 距上一帧时间，用于动画计算
        // render_function: 用户自定义渲染函数，用于描述UI布局和组件，同时处理UI交互，传入的canvas是管理整个窗口的画布
        virtual void new_frame(uint32_t width, uint32_t height, float delta_t,
                            const std::function<void(canvas&)>& render_function) = 0;
        // 重置内部状态，包括中间状态存储和纹理分配器的纹理引用
        virtual void reset_cache() = 0;
        // 加载图片，转发至纹理分配器
        virtual texture_region load_image(const image_desc& image, float max_scale) = 0;
        // 加载字体，转发至字体后端
        [[nodiscard]] virtual std::shared_ptr<font> load_font(const std::pmr::string& name, float height) const = 0;
        // 获取风格配置的引用
        virtual style& global_style() noexcept = 0;
        // 获取最近几十帧的性能统计信息，具体信息参见pipeline_staticstics结构体定义
        [[nodiscard]] virtual const pipeline_statistics& statistics() noexcept = 0;
    };

    // 使用选中的组件构建animgui上下文，构建后仅需操作render_backend和context本身
    std::unique_ptr<context>
    create_animgui_context(input_backend& input_backend, render_backend& render_backend, font_backend& font_backend,
                        emitter& emitter, animator& animator, command_optimizer& command_optimizer,
                        image_compactor& image_compactor,
                        std::pmr::memory_resource* memory_manager = std::pmr::get_default_resource());
