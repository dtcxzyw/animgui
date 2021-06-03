// SPDX-License-Identifier: MIT

#include <GLFW/glfw3.h>
#include <animgui/backends/glfw3.hpp>
#include <animgui/backends/opengl3.hpp>
#include <animgui/backends/stbfont.hpp>
#include <animgui/builtins/animators.hpp>
#include <animgui/builtins/emitters.hpp>
#include <animgui/builtins/layouts.hpp>
#include <animgui/builtins/widgets.hpp>
#include <animgui/core/canvas.hpp>
#include <animgui/core/context.hpp>
#include <animgui/core/font_backend.hpp>
#include <animgui/core/style.hpp>
#include <iostream>

void fail(const std::string& str) {
    std::cout << str << std::endl;
    std::terminate();
}

void render(animgui::canvas& canvas) {
    animgui::layout_row(canvas, animgui::row_alignment::left, [](animgui::row_layout_canvas& layout) {
        animgui::text(layout, "Hello World");
        layout.newline();
        if(animgui::button_label(layout, "Exit")) {
            std::exit(0);
        }
        animgui::text(layout, "test1");
        animgui::text(layout, "text2");
    });
}

int main() {
    glfwSetErrorCallback([](const int id, const char* error) { fail("[GLFW:" + std::to_string(id) + "] " + error); });
    if(!glfwInit())
        fail("Failed to initialize glfw");

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#ifdef ANIMGUI_MACOS
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    GLFWwindow* const window = glfwCreateWindow(800, 600, "Animgui demo", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    {
        std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource();
        const auto glfw3_backend = animgui::create_glfw3_backend(window);
        const auto ogl3_backend = animgui::create_opengl3_backend();
        const auto stb_font_backend = animgui::create_stb_font_backend();
        const auto font = stb_font_backend->load_font("times", 18.0f);
        const auto animator = animgui::create_dummy_animator();
        const auto emitter = animgui::create_builtin_emitter(memory_resource);
        auto ctx = animgui::create_animgui_context(*glfw3_backend, *ogl3_backend, *emitter, *animator, memory_resource);
        auto&& style = ctx->style();
        style.default_font = style.fallback_font = font;

        glfwSwapInterval(0);

        auto last = glfwGetTime();
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);

            const auto current = glfwGetTime();
            const auto delta_t = static_cast<float>(current - last);
            last = current;

            ctx->new_frame(w, h, delta_t, render);

            ogl3_backend->emit();
            glfwSwapBuffers(window);
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
