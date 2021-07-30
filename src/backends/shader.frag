// SPDX-License-Identifier: MIT
#version 450 core

layout (location = 0) in vec2 f_tex_coord;
layout (location = 1) in vec4 f_color;

layout(location = 0) out vec4 out_frag_color;

layout(binding = 0) uniform sampler2D tex;

void main() {
    out_frag_color = texture(tex, f_tex_coord) * f_color;
}
