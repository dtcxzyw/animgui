// SPDX-License-Identifier: MIT

#include <animgui/backends/opengl3.hpp>

#include <animgui/core/render_backend.hpp>

#include <gl/glew.h>

namespace animgui {

    static const char* const shader_vert_src = R"(

        #version 330 core

        layout (location = 0) in vec2 pos;
        layout (location = 1) in vec2 tex_coord;
        layout (location = 2) in vec4 color;

        out vec2 f_tex_coord;
        out vec4 f_color;

        void main() {
        	gl_Position = vec4(pos, 0.0f, 1.0f);
            f_tex_coord = tex_coord;
            f_color = color;
        } 

        )";

    static const char* shader_frag_src = R"(

        #version 330 core

        in vec2 f_tex_coord;
        in vec4 f_color;

        out vec4 out_frag_color;

        uniform sampler2D texture;

        void main() {
         	out_frag_color = texture(texture, f_tex_coord) * f_color;
        }

        )";

    class texture_impl final : public texture {

        unsigned int m_id;
        channel m_channel;
        uvec2 m_size;
        bool m_own;
        GLenum m_format;

        static GLenum get_format(const channel channel) {
            if(channel == channel::alpha)
                return GL_ALPHA;
            else if(channel == channel::rgb)
                return GL_RGB;
            else
                return GL_RGBA;
        }

    public:
        explicit texture_impl(const texture_impl&) = delete;
        explicit texture_impl(texture_impl&&) = delete;
        texture_impl& operator=(const texture_impl&) = delete;
        texture_impl& operator=(texture_impl&&) = delete;

        texture_impl(const channel channel, const uvec2 size)
            : m_id(0), m_channel(channel), m_size(size), m_own(true), m_format(get_format(channel)) {
            glGenTextures(1, &m_id);
            glBindTexture(GL_TEXTURE_2D, m_id);
            glTexImage2D(GL_TEXTURE_2D, 0, m_format, size.x, size.y, 0, m_format, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        texture_impl(unsigned int handel, channel channel, uvec2 size)
            : m_id(handel), m_channel(channel), m_size(size), m_own(false), m_format(get_format(channel)) {}

        ~texture_impl() override {
            if(m_own) {
                glDeleteTextures(1, &m_id);
            }
        }

        void update_texture(const uvec2 offset, const image_desc& image) override {
            glBindTexture(GL_TEXTURE_2D, m_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, offset.x, offset.y, image.size.x, image.size.y, m_format, GL_UNSIGNED_BYTE,
                            image.data);
        }

        [[nodiscard]] uvec2 texture_size() const noexcept override {
            return m_size;
        }

        [[nodiscard]] channel channels() const noexcept override {
            return m_channel;
        }

        [[nodiscard]] uint64_t native_handle() const noexcept override {
            return m_id;
        }
    };

    class render_backend_impl final : public render_backend {

        std::pmr::vector<command> m_command_list;
        animgui::cursor m_cursor;
        unsigned int m_program_id;
        unsigned int m_vbo;
        unsigned int m_vao;
        texture_impl m_empty;

        static void check_compile_errors(const GLuint shader, const std::string& type) {
            GLint success;
            GLchar infoLog[1024];
            if(type != "PROGRAM") {
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if(!success) {
                    glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                    throw std::runtime_error("ERROR::SHADER_COMPILATION_ERROR of type: " + type + "\n" + infoLog + "\n");
                }
            } else {
                glGetProgramiv(shader, GL_LINK_STATUS, &success);
                if(!success) {
                    glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                    throw std::runtime_error("ERROR::PROGRAM_LINKING_ERROR of type: " + type + "\n" + infoLog + "\n");
                }
            }
        }

        static GLenum get_mode(const primitive_type type) {
            switch(type) {
                case primitive_type::points:
                    return GL_POINTS;
                case primitive_type::lines:
                    return GL_LINES;
                case primitive_type::line_strip:
                    return GL_LINE_STRIP;
                case primitive_type::line_loop:
                    return GL_LINE_LOOP;
                case primitive_type::triangles:
                    return GL_TRIANGLES;
                case primitive_type::triangle_fan:
                    return GL_TRIANGLE_FAN;
                case primitive_type::triangle_strip:
                    return GL_TRIANGLE_STRIP;
                case primitive_type::quads:
                    return GL_QUADS;
            }
            return 0;
        }

    public:
        render_backend_impl() : m_cursor(cursor::arrow), m_empty(channel::rgba, uvec2{1, 1}) {
            const unsigned int shader_vert = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(shader_vert, 1, &shader_vert_src, nullptr);
            glCompileShader(shader_vert);
            check_compile_errors(shader_vert, "VERTEX");

            const unsigned int shader_frag = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(shader_frag, 1, &shader_frag_src, nullptr);
            glCompileShader(shader_frag);
            check_compile_errors(shader_frag, "FRAGMENT");

            m_program_id = glCreateProgram();
            glAttachShader(m_program_id, shader_vert);
            glAttachShader(m_program_id, shader_frag);
            glLinkProgram(m_program_id);
            check_compile_errors(m_program_id, "PROGRAM");

            glDeleteShader(shader_vert);
            glDeleteShader(shader_frag);

            glGenBuffers(1, &m_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(4 * sizeof(float)));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            unsigned char data[4] = { 255, 255, 255, 255 };
            m_empty.update_texture(uvec2{ 0, 0 }, animgui::image_desc{ 1, 1, channel::rgba, data});
        }

        void update_command_list(std::pmr::vector<command> command_list) override {
            m_command_list = std::move(command_list);
        }

        void set_cursor(const animgui::cursor cursor) noexcept override {
            m_cursor = cursor;
        }

        [[nodiscard]] animgui::cursor cursor() const noexcept override {
            return m_cursor;
        }

        std::shared_ptr<texture> create_texture(const uvec2 size, const channel channels) override {
            return std::make_shared<texture_impl>(channels, size);
        }

        std::shared_ptr<texture> create_texture_from_native_handle(const uint64_t handle, const uvec2 size,
                                                                   const channel channels) override {
            return std::make_shared<texture_impl>(static_cast<uint32_t>(handle), channels, size);
        }

        void emit(const uvec2 screen_size) override {
            for(auto& [clip, desc] : m_command_list) {
                const int left = static_cast<int>(std::floor(clip.left));
                const int right = static_cast<int>(std::ceil(clip.right));
                const int bottom = static_cast<int>(std::ceil(clip.bottom));
                const int top = static_cast<int>(std::floor(clip.top));

                glScissor(left, screen_size.y - bottom, right - left, bottom - top);
                if(desc.index() == 0) {
                    auto&& drawback = std::get<0>(desc);
                    drawback.callback(clip);
                } else {
                    glEnable(GL_BLEND);
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_CULL_FACE);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    auto&& primitives = std::get<1>(desc);
                    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                    glBufferData(GL_ARRAY_BUFFER, primitives.vertices.size() * sizeof(vertex), primitives.vertices.data(),
                                 GL_STREAM_DRAW);
                    glLineWidth(primitives.point_line_size);
                    glPointSize(primitives.point_line_size);
                    glBindVertexArray(m_vao);
                    glActiveTexture(GL_TEXTURE0);
                    if(!primitives.texture) {
                        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_empty.native_handle()));
                    } else {
                        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(primitives.texture->native_handle()));
                    }

                    glDrawArrays(get_mode(primitives.type), 0, primitives.vertices.size());
                }
            }
        }
    };

    std::shared_ptr<render_backend> create_opengl3_backend() {
        return std::make_shared<render_backend_impl>();
    }

}  // namespace animgui
