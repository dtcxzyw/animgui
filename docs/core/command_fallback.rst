指令转换器
===================================

有的渲染后端不支持某些图元的完整功能（比如任意大小的点和线），这时就必须将原始指令转换为渲染后端支持的指令。

其转换路径如下 (可能存在多步转换)：
1. points -> quads
2. lines -> quads
3. line_strip -> lines
4. line_loop -> lines
5. triangle_fan -> triangles
6. triangle_strip -> triangles
7. quads -> triangle_strip

由上述路径可知，渲染后端必须支持triangles格式的绘制指令。
