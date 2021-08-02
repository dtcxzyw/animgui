// SPDX-License-Identifier: MIT

#pragma once
#include <animgui/core/common.hpp>
#include <string_view>

namespace animgui {
    using namespace std::literals;

    static auto vertex_shader_src = R"(
        cbuffer constant_buffer : register(b0) {
            float2 size;
            int mode;
            int padding;
        };
        struct VS_INPUT {
            float2 pos : POSITION;
            float2 tex_coord  : TEXCOORD0;
            float4 color : COLOR0;
        };
            
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex_coord  : TEXCOORD0;
            float4 color : COLOR0;
        };

        PS_INPUT main(VS_INPUT input) {
            PS_INPUT output;
            output.pos = float4(input.pos.x/size.x*2.0f-1.0f,1.0f-input.pos.y/size.y*2.0f, 0.0f, 1.0f);
            output.tex_coord  = input.tex_coord;
            output.color = input.color;
            return output;
        }
    )"sv;

    static auto pixel_shader_src = R"(
        cbuffer constant_buffer : register(b0) {
            float2 size;
            int mode;
            int padding;
        };
        struct PS_INPUT {
            float4 pos : SV_POSITION;
            float2 tex_coord  : TEXCOORD0;
            float4 color : COLOR0;
        };
        sampler sampler0;
        Texture2D texture0;
        
        float4 main(PS_INPUT input) : SV_Target {
            if(mode==0)
                return input.color * texture0.Sample(sampler0, input.tex_coord);
            if(mode==1)
                return input.color * texture0.Sample(sampler0, input.tex_coord).xxxx;
            return input.color;
        }
    )"sv;

    struct constant_buffer final {
        vec2 size;
        int mode;
        int padding;
    };

    static_assert(sizeof(constant_buffer) % 16 == 0);
}  // namespace animgui
