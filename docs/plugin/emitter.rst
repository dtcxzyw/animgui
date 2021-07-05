扩展指令发射器
===================================

参见<animgui/core/emitter.hpp>:

扩展指令发射器需要派生自emitter并实现以下接口：

.. code-block:: c++

    class emitter {
    public:
        // 将图元转换为绘制指令
        // size: 窗口大小，用于裁剪。
        // operations: 操作序列，操作有三种类型：
        // push_region: 将当前区域入栈，根据父区域的参数可计算得到当前区域的绘制起始偏移和裁剪矩形
        // pop_region: 将当前区域出栈
        // primitive: 在当前区域中绘制基本图元，如文本、矩形等
        // style: 风格设置
        // font_callback: 用于渲染文字时动态加载文字
        // 返回值：返回渲染后端识别的command，有point/line/triangle/quad等类型
        virtual std::pmr::vector<command> transform(vec2 size, span<operation> operations, const style& style,
                                                    const std::function<texture_region(font&, glyph_id)>& font_callback) = 0;

        // 计算图元所占的大小，用于计算布局
        // primitive: 基本图元的描述，如文本、矩形等
        // style: 风格设置
        virtual vec2 calculate_bounds(const primitive& primitive, const style& style) = 0;
    };


具体示例可参考builtins/emitters.cpp。
