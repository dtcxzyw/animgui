扩展纹理分配器
===================================

参见<animgui/core/image_compactor.hpp>:

扩展纹理分配器器需要派生自image_compactor并实现以下接口：

.. code-block:: c++

    class image_compactor {
    public:
        // 清空内部状态
        virtual void reset() = 0;
        // 为image分配位置并将image提交到渲染后端上
        // image: 图片描述，参见结构体image_desc
        // max_scale: 图片最大缩小倍数，用于调整虚拟纹理的margin以避免mipmap造成不同纹理间相互干扰
        // 返回值：纹理引用和对应的四角uv坐标
        virtual texture_region compact(const image_desc& image, float max_scale) = 0;
    };

具体示例可参考builtins/image_compactors.cpp。
