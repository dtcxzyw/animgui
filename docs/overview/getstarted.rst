入门指南
===================================

系统需求
-----------------------------------

- 支持C++17的C++编译器(GCC 7+ 或MSVC 19.14+)
- 支持OpenGL3.3或Direct3D 11
- CMake 3.19+

安装
-----------------------------------

安装依赖库
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- 核心依赖：utfcpp
- glfw3后端依赖：glfw3
- opengl后端依赖：glew
- stb_font后端依赖：stb

推荐使用vcpkg安装所有依赖。

clone并编译animgui
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    git clone https://github.com/dtcxzyw/animgui.git
    cd animgui
    mkdir build
    cd build
    cmake .. [-D definitions]

然后执行make或者打开sln解决方案构建并安装animgui

第一个animgui程序
-----------------------------------

以glfw3+opengl3+stb_font后端为例：

.. code-block:: c++

    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
    #include <animgui/backends/glfw3.hpp>
    #include <animgui/backends/opengl3.hpp>
    #include <animgui/backends/stbfont.hpp>
    #include <animgui/builtins/animators.hpp>
    #include <animgui/builtins/command_optimizers.hpp>
    #include <animgui/builtins/emitters.hpp>
    #include <animgui/builtins/image_compactors.hpp>
    #include <animgui/builtins/layouts.hpp>
    #include <animgui/builtins/widgets.hpp>
    #include <animgui/core/context.hpp>
    #include <animgui/core/input_backend.hpp>
    #include <iostream>
    #include <string>

    [[noreturn]] void fail(const std::string& str) {
        std::cout << str << std::endl;
        std::terminate();
    }

    class demo final {
        // 初始化字体
        explicit demo(context& ctx) {
            auto&& style = context.global_style();
            style.default_font = context.load_font("msyh", 30.0f);
        }
        // 每帧调用的渲染函数
        void render(canvas& canvas_root) override {
            // 居中布局
            layout_row_center(canvas_root, [&](row_layout_canvas& layout) {
                text(layout, "Hello World 你好 世界");// 增加一个标签
                layout.newline();// 换行，使得上面的标签独立成行
                if(button(layout,"退出"))// 增加一个按钮，如果按钮被按下则返回true，否则为false
                    layout.input().close_window();// 通过窗口管理器关闭窗口
            });
        }
    };

    int main() {
        // 初始化窗口
        glfwSetErrorCallback([](const int id, const char* error) { fail("[GLFW:" + std::to_string(id) + "] " + error); });
        if(!glfwInit())
            fail("Failed to initialize glfw");

        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_DEPTH_BITS, GL_FALSE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    #ifdef ANIMGUI_MACOS
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    #endif
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 8);

        const int width = 1024, height = 768;
        GLFWwindow* const window = glfwCreateWindow(width, height, "Animgui demo ( opengl3_glfw3 )", nullptr, nullptr);
        int screen_w, screen_h;
        glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), nullptr, nullptr, &screen_w, &screen_h);
        glfwSetWindowPos(window, (screen_w - width) / 2, (screen_h - height) / 2);

        glfwMakeContextCurrent(window);

        if(glewInit() != GLEW_OK)
            fail("Failed to initialize glew");

        {
            // 初始化组件
            std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();
            std::function<void()> draw;
            const auto glfw3_backend = animgui::create_glfw3_backend(window, draw);
            const auto ogl3_backend = animgui::create_opengl3_backend();
            const auto stb_font_backend = animgui::create_stb_font_backend(8.0f);
            const auto animator = animgui::create_dummy_animator();
            const auto emitter = animgui::create_builtin_emitter(memory_resource);
            const auto command_optimizer = animgui::create_builtin_command_optimizer();
            const auto image_compactor = animgui::create_builtin_image_compactor(*ogl3_backend, memory_resource);
            // 根据选好的组件初始化上下文，此后用户仅需调用context和render_backend的方法
            auto ctx = animgui::create_animgui_context(*glfw3_backend, *ogl3_backend, *stb_font_backend, *emitter, *animator,
                                                    *command_optimizer, *image_compactor, memory_resource);
            const auto app = std::make_unique<demo>(*ctx);

            glfwSwapInterval(0);

            auto last = glfwGetTime();

            draw = [&] {
                int w, h;
                glfwGetFramebufferSize(window, &w, &h);
                if(w == 0 || h == 0)
                    return;

                const auto current = glfwGetTime();
                const auto delta_t = static_cast<float>(current - last);
                last = current;

                int window_w, window_h;
                glfwGetWindowSize(window, &window_w, &window_h);

                // 主入口
                ctx->new_frame(window_w, window_h, delta_t, [&](animgui::canvas& canvas_root) { app->render(canvas_root); });

                glViewport(0, 0, w, h);
                glScissor(0, 0, w, h);
                glClearColor(0, 0, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);

                // 提交渲染指令
                ogl3_backend->emit(animgui::uvec2{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });
                glfwSwapBuffers(window);
            };

            // 主循环
            while(!glfwWindowShouldClose(window)) {
                glfw3_backend->new_frame();
                draw();
            }
        }
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }


将代码拷贝至demo.cpp，并编译运行，可以看到出现一个窗口，窗口内有一行文字和一个按钮。
阅读其它文档以开始使用animgui。
