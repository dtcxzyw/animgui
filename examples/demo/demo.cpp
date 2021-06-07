// SPDX-License-Identifier: MIT

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
#include <animgui/core/canvas.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/font_backend.hpp>
#include <animgui/core/style.hpp>
#include <iostream>

[[noreturn]] void fail(const std::string& str) {
    std::cout << str << std::endl;
    std::terminate();
}

void render(animgui::canvas& canvas) {
    animgui::layout_row_center(canvas, [](animgui::row_layout_canvas& layout) {
        animgui::text(layout, "Hello World 你好 世界");
        layout.newline();
        static uint32_t count = 0;
        animgui::text(layout, "Click: " + std::pmr::string{ std::to_string(count) });
        if(animgui::button_label(layout, "Add")) {
            ++count;
        }
        layout.newline();
        if(animgui::button_label(layout, "Exit")) {
            std::exit(0);
        }
    });
}

// refer to https://learnopengl.com/In-Practice/Debugging
void glDebugOutput(const GLenum source, const GLenum type, const unsigned int id, const GLenum severity, GLsizei,
                   const char* message, const void*) {
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch(source) {
        case GL_DEBUG_SOURCE_API:
            std::cout << "Source: API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            std::cout << "Source: Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            std::cout << "Source: Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            std::cout << "Source: Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            std::cout << "Source: Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            std::cout << "Source: Other";
            break;
    }
    std::cout << std::endl;

    switch(type) {
        case GL_DEBUG_TYPE_ERROR:
            std::cout << "Type: Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            std::cout << "Type: Deprecated Behaviour";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            std::cout << "Type: Undefined Behaviour";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            std::cout << "Type: Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            std::cout << "Type: Performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            std::cout << "Type: Marker";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            std::cout << "Type: Push Group";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            std::cout << "Type: Pop Group";
            break;
        case GL_DEBUG_TYPE_OTHER:
            std::cout << "Type: Other";
            break;
    }
    std::cout << std::endl;

    switch(severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            std::cout << "Severity: high";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cout << "Severity: medium";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            std::cout << "Severity: low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cout << "Severity: notification";
            break;
    }
    std::cout << std::endl;
    std::cout << std::endl;
}

int main() {
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

    GLFWwindow* const window = glfwCreateWindow(800, 600, "Animgui demo", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    if(glewInit() != GLEW_OK)
        fail("Failed to initialize glew");

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if(flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    {
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();
        std::function<void()> draw;
        const auto glfw3_backend = animgui::create_glfw3_backend(window, draw);
        const auto ogl3_backend = animgui::create_opengl3_backend();
        const auto stb_font_backend = animgui::create_stb_font_backend();
        const auto font = stb_font_backend->load_font("msyh", 100.0f);
        const auto animator = animgui::create_dummy_animator();
        const auto emitter = animgui::create_builtin_emitter(memory_resource);
        const auto command_optimizer = animgui::create_builtin_command_optimizer();
        const auto image_compactor = animgui::create_builtin_image_compactor(*ogl3_backend, memory_resource);
        auto ctx = animgui::create_animgui_context(*glfw3_backend, *ogl3_backend, *emitter, *animator, *command_optimizer,
                                                   *image_compactor, memory_resource);
        auto&& style = ctx->style();
        style.font = font;

        glfwSwapInterval(0);

        auto last = glfwGetTime();

        draw = [&] {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);

            const auto current = glfwGetTime();
            const auto delta_t = static_cast<float>(current - last);
            last = current;

            ctx->new_frame(w, h, delta_t, render);

            glViewport(0, 0, w, h);
            glScissor(0, 0, w, h);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ogl3_backend->emit(animgui::uvec2{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });
            glfwSwapBuffers(window);
        };

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            draw();
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
