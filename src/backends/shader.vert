// SPDX-License-Identifier: MIT
#version 450 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 tex_coord;
layout (location = 2) in vec4 color;

layout (location = 0) out vec2 f_tex_coord;
layout (location = 1) out vec4 f_color;

layout(constant_id = 0) const float size_x = 1.0f;
layout(constant_id = 1) const float size_y = 1.0f;

void main() {
    gl_Position = vec4(pos.x/size_x*2.0f-1.0f,pos.y/size_y*2.0f-1.0f, 0.0f, 1.0f);
    f_tex_coord = tex_coord;
    f_color = color;
}
