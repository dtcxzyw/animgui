// SPDX-License-Identifier: MIT

#pragma once
#include "common.hpp"

namespace animgui {
    enum class key_code : uint32_t {
        left_button = 0x01,
        right_button = 0x02,
        middle_button = 0x04,
        back = 0x08,
        tab = 0x09,
        shift = 0x10,
        control = 0x11,
        menu = 0x12,
        pause = 0x13,
        capital = 0x14,
        escape = 0x1B,
        space = 0x20,
        end = 0x23,
        home = 0x24,
        left = 0x25,
        up = 0x26,
        right = 0x27,
        down = 0x28,
        select = 0x29,
        print = 0x2A,
        insert = 0x2D,
        delete_ = 0x2E,
        num_0 = 0x30,
        num_1 = 0x31,
        num_2 = 0x32,
        num_3 = 0x33,
        num_4 = 0x34,
        num_5 = 0x35,
        num_6 = 0x36,
        num_7 = 0x37,
        num_8 = 0x38,
        num_9 = 0x39,
        alpha_a = 0x41,
        alpha_b = 0x42,
        alpha_c = 0x43,
        alpha_d = 0x44,
        alpha_e = 0x45,
        alpha_f = 0x46,
        alpha_g = 0x47,
        alpha_h = 0x48,
        alpha_i = 0x49,
        alpha_j = 0x4A,
        alpha_k = 0x4B,
        alpha_l = 0x4C,
        alpha_m = 0x4D,
        alpha_n = 0x4E,
        alpha_o = 0x4F,
        alpha_p = 0x50,
        alpha_q = 0x51,
        alpha_r = 0x52,
        alpha_s = 0x53,
        alpha_t = 0x54,
        alpha_u = 0x55,
        alpha_v = 0x56,
        alpha_w = 0x57,
        alpha_x = 0x58,
        alpha_y = 0x59,
        alpha_z = 0x5A,
        f1 = 0x70,
        f2 = 0x71,
        f3 = 0x72,
        f4 = 0x73,
        f5 = 0x74,
        f6 = 0x75,
        f7 = 0x76,
        f8 = 0x77,
        f9 = 0x78,
        f10 = 0x79,
        f11 = 0x7A,
        f12 = 0x7B,
        left_shift = 0xA0,
        right_shift = 0xA1,
        left_control = 0xA2,
        right_control = 0xA3,
        left_menu = 0xA4,
        right_menu = 0xA5
    };

    class input_backend {
    public:
        input_backend() = default;
        input_backend(const input_backend&) = delete;
        input_backend(input_backend&&) = default;
        input_backend& operator=(const input_backend&) = delete;
        input_backend& operator=(input_backend&&) = default;
        virtual ~input_backend() = default;

        virtual void set_clipboard_text(std::pmr::string str) = 0;
        virtual std::pmr::string get_clipboard_text() = 0;

        virtual vec2 get_cursor_pos() = 0;
        virtual bool get_key(key_code code) = 0;
        // TODO: game pad
    };
}  // namespace animgui
