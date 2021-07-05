UI布局原理
===================================

canvas系统
-----------------------------------

canvas管理一块区域内的绘制、动画、用户交互以及中间状态。

canvas仅处理region和primitive。primitive即基本图元，如文字、矩形、线条等，是绘制的基本单元；region则是primitive和子region的容器，主要提供裁剪矩形参数，是处理用户输入的基本单元。

region和primitive通过identifier（uint64_t）唯一表示，主要用于识别两帧之间同一对象，利用identifier可以存储中间状态在全局哈希表中。

animgui本身没有布局系统的专用类，而是通过扩展canvas的功能来提供布局功能。

除了顶层canvas外，其余canvas实例均通过调用顶层canvas的功能来实现特殊功能。

.. code-block:: c++

    class canvas {
    public:

        // 计算图元的大小，简单转发至内部emitter
        [[nodiscard]] virtual vec2 calculate_bounds(const primitive& primitive) const = 0;

        // 生成当前region的子标识符，即在相同region下，第i个region_sub_uid和上一帧的第i个region_sub_uid是相等的，用于创建匿名region，多用于UI组件
        [[nodiscard]] virtual identifier region_sub_uid() = 0;

        // 将当前区域压入栈中
        // uid: 唯一标识符，将与父区域的uid混合得到真实标识符，可视为目录名
        // reserved_bounds: 预分配区域包围盒，若不支持自动布局或者子区域需要知道预分配区域，则必须指定此值；否则可在pop_region中再次指定该值
        // 返回值：push_region在命令缓冲中的下标，可通过commands()访问并修改；该region的真实标识符，用于中间状态存储，可视为目录绝对路径
        virtual std::pair<size_t, identifier> push_region(identifier uid, const std::optional<bounds_aabb>& reserved_bounds = std::nullopt) = 0;

        // 将当前区域弹出栈
        // new_bounds: 自动布局计算的最终区域包围盒。push_region与pop_region至少要指定一次包围盒
        virtual void pop_region(const std::optional<bounds_aabb>& new_bounds = std::nullopt) = 0;

        // 在当前区域中绘制图元
        // uid: 图元的唯一标识符，将与父区域的uid混合得到真实标识符，可视为文件名
        // primitive: 图元信息，如文字、矩形等。
        // 返回值：该primitive在命令缓冲中的下标，可通过commands()访问并修改；该primitive的真实标识符，用于中间状态存储，可视为文件绝对路径
        virtual std::pair<size_t, identifier> add_primitive(identifier uid, primitive primitive) = 0;

        // 当前区域的预分配区域大小，由push_region指定
        [[nodiscard]] virtual vec2 reserved_size() const noexcept = 0;

        // 当前指令缓冲
        virtual span<operation> commands() noexcept = 0;

        // 根据唯一标识符获取一个中间状态的引用。如果该状态尚未初始化，则调用默认初始化。
        // 注意：每帧相同对象的唯一标识符应该相等
        // 注意：同一个uid不能存储多类型的状态，可通过mix与""_id生成子标识符来存储多类型的状态

        template <typename T>
        T& storage(const identifier uid);

        // 获取本帧风格配置
        [[nodiscard]] virtual const style& global_style() const noexcept = 0;

        // 获取当前内存分配器，一般是arena allocator
        [[nodiscard]] virtual std::pmr::memory_resource* memory_resource() const noexcept = 0;

        // 获取输入后端
        [[nodiscard]] virtual input_backend& input() const noexcept = 0;

        // 获取当前区域的估计包围盒（源自上一帧的数据）
        [[nodiscard]] virtual const bounds_aabb& region_bounds() const = 0;

        // 获取当前区域的绝对偏移
        [[nodiscard]] virtual vec2 region_offset() const = 0;

        // 判断鼠标是否悬停在当前区域上
        [[nodiscard]] virtual bool region_hovered() const = 0;

        // 判断鼠标是否悬停在指定区域上
        [[nodiscard]] virtual bool hovered(const bounds_aabb& bounds) const = 0;

        // 声明当前区域是可获取焦点的，用于游戏手柄的焦点移动功能。接口尚不稳定，故此处不介绍。
        [[nodiscard]] virtual bool region_request_focus(bool force = false) = 0;

        // 动画系统尚未完成，故此处不介绍
        [[nodiscard]] virtual float step(identifier id, float dest) = 0;
    };


简单UI布局实现
-----------------------------------

简单地利用region并重用已有布局函数来实现新的布局。

比如下面的layout_row_center (animgui/builtins/layouts.cpp)

.. code-block:: c++

    bounds_aabb layout_row_center(canvas& parent, const std::function<void(row_layout_canvas&)>& render_function) {
        // 压入新region，暂不指定包围盒
        parent.push_region("layout_center_region"_id);
        // 由用户定义区域内的组件，并拿到区域内组件的包围盒大小
        const auto [w, h] = layout_row(parent, row_alignment::middle, render_function);
        // 计算真实包围盒，以将区域内的所有内容移动至中心
        const auto offset_y = (parent.reserved_size().y - h) / 2.0f;
        const auto bounds = bounds_aabb{ 0.0f, parent.reserved_size().x, offset_y, offset_y + h };
        // 弹出region，指定包围盒
        parent.pop_region(bounds);
        // 返回region的包围盒
        return bounds;
    }

复杂UI布局实现
-----------------------------------

复杂UI布局则需要定义新的canvas，给canvas提供新功能并hook canvas的某些方法。

为了减少代码重复，layouts.hpp中提供了layout_proxy用于默认方法转发，可以理解为动态多态。
警告：与真正的多态不同的是，父方法调用canvas的某个方法时，该方法无法被子方法覆写。原因很简单但这一点很容易被忽略。即不支持自上而下调用时的多态。

具体示例参见layout_row的实现 (animgui/builtins/layouts.cpp)
