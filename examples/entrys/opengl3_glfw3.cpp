// SPDX-License-Identifier: MIT

#include "../application.hpp"
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
#include <animgui/core/context.hpp>
#include <animgui/core/input_backend.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

[[noreturn]] void fail(const std::string& str) {
    std::cout << str << std::endl;
    std::_Exit(EXIT_FAILURE);
}

// refer to https://learnopengl.com/In-Practice/Debugging
void gl_debug_output(const GLenum source, const GLenum type, const unsigned int id, const GLenum severity, GLsizei,
                     const char* message, const void*) {
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
    switch(source) {  // NOLINT(hicpp-multiway-paths-covered)
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

    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
    switch(type) {  // NOLINT(hicpp-multiway-paths-covered)
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

    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
    switch(severity) {  // NOLINT(hicpp-multiway-paths-covered)
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
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
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
    GLFWwindow* const window = glfwCreateWindow(width, height, "Animgui demo (opengl3_glfw3)", nullptr, nullptr);
    int screen_w, screen_h;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), nullptr, nullptr, &screen_w, &screen_h);
    glfwSetWindowPos(window, (screen_w - width) / 2, (screen_h - height) / 2);

    glfwMakeContextCurrent(window);

    if(glewInit() != GLEW_OK)
        fail("Failed to initialize glew");

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if(flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_output, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    {
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();
        std::function<void()> draw;
        const auto glfw3_backend = animgui::create_glfw3_backend(window, draw);
        const auto ogl3_backend = animgui::create_opengl3_backend();
        const auto stb_font_backend = animgui::create_stb_font_backend(8.0f);
        const auto animator = animgui::create_dummy_animator();
        const auto emitter = animgui::create_builtin_emitter(memory_resource);
        const auto command_optimizer = animgui::create_builtin_command_optimizer();
        const auto image_compactor = animgui::create_builtin_image_compactor(*ogl3_backend, memory_resource);
        auto ctx = animgui::create_animgui_context(*glfw3_backend, *ogl3_backend, *stb_font_backend, *emitter, *animator,
                                                   *command_optimizer, *image_compactor, memory_resource);
        const auto app = animgui::create_demo_application(*ctx);

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

            ctx->new_frame(window_w, window_h, delta_t, [&](animgui::canvas& canvas_root) { app->render(canvas_root); });

            glViewport(0, 0, w, h);
            glScissor(0, 0, w, h);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            ogl3_backend->emit(animgui::uvec2{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) });
            glfwSwapBuffers(window);
        };

        while(!glfwWindowShouldClose(window)) {
            glfw3_backend->new_frame();
            draw();
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
