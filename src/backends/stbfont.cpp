// SPDX-License-Identifier: MIT

#include <animgui/core/common.hpp>
#include <animgui/core/font_backend.hpp>
#include <animgui/core/render_backend.hpp>
#include <filesystem>
#include <fstream>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace fs = std::filesystem;

namespace animgui {
    // TODO: handle character advance/positioning
    class font_impl final : public font {
        stbtt_fontinfo m_font_info;
        float m_height, m_scale;

    public:
        font_impl(const fs::path& path, const float height, std::pmr::memory_resource* memory_resource)
            : m_font_info{}, m_height{ height }, m_scale{ 0.0f } {
            std::pmr::vector<uint8_t> buffer{ fs::file_size(path), memory_resource };
            {
                std::ifstream in{ path, std::ios::in | std::ios::binary };
                in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
                in.close();
            }
            stbtt_InitFont(&m_font_info, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0));
            m_scale = stbtt_ScaleForPixelHeight(&m_font_info, height);
        }
        [[nodiscard]] float height() const noexcept override {
            return m_height;
        }
        void render_to_bitmap(const uint32_t codepoint, const image_desc& dest) const override {
            stbtt_MakeCodepointBitmap(&m_font_info, const_cast<uint8_t*>(static_cast<const uint8_t*>(dest.data)), dest.size.x,
                                      dest.size.y, dest.size.x, m_scale, m_scale, codepoint);
        }
        [[nodiscard]] float calculate_width(const uint32_t codepoint) const override {
            int x0, y0, x1, y1;
            stbtt_GetCodepointBitmapBox(&m_font_info, codepoint, m_scale, m_scale, &x0, &y0, &x1, &y1);
            return static_cast<float>(x1 - x0);
        }
        [[nodiscard]] bool exists(const uint32_t codepoint) const override {
            return stbtt_FindGlyphIndex(&m_font_info, codepoint);
        }
    };
    class stb_font_backend final : public font_backend {
        [[nodiscard]] static fs::path locate_font(const fs::path& dir, const std::pmr::string& name) {
            const auto path = dir / name;
            if(auto ttf = (path.string() + ".ttf"); fs::is_regular_file(ttf))
                return ttf;
            if(auto ttc = (path.string() + ".ttc"); fs::is_regular_file(ttc))
                return ttc;
            return {};
        }
        [[nodiscard]] static fs::path locate_font(const std::pmr::string& name) {
#if defined(ANIMGUI_WINDOWS)
            if(auto path = locate_font("C:/Windows/Fonts", name); !path.empty())
                return path;
#elif defined(ANIMGUI_LINUX)
            if(auto path = locate_font("~/.local/share/fonts", name); !path.empty())
                return path;
            if(auto path = locate_font("/usr/share/fonts", name); !path.empty())
                return path;
#elif defined(ANIMGUI_MACOS)
            if(auto path = locate_font("/System/Library/Fonts", name); !path.empty())
                return path;
#endif
            if(auto path = locate_font(".", name); !path.empty())
                return path;
            return {};
        }

    public:
        [[nodiscard]] std::shared_ptr<font> load_font(const std::pmr::string& name, float height) const override {
            const auto path = fs::is_regular_file(name) ? fs::path{ name } : locate_font(name);
            if(path.empty())
                throw std::logic_error{ std::string{ "Failed to find font " + name } };
            return std::make_shared<font_impl>(path, height, name.get_allocator().resource());
        }
    };
    std::shared_ptr<font_backend> create_stb_font_backend() {
        return std::make_shared<stb_font_backend>();
    }
}  // namespace animgui
