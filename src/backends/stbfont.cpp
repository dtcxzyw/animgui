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
    class font_impl final : public font {
        std::pmr::vector<uint8_t> m_font_data;
        stbtt_fontinfo m_font_info;
        float m_height, m_super_sample, m_render_scale, m_bake_scale, m_line_spacing, m_baseline, m_standard_width;

    public:
        font_impl(const fs::path& path, const float height, const float super_sample, std::pmr::memory_resource* memory_resource)
            : m_font_data{ fs::file_size(path), memory_resource }, m_font_info{}, m_height{ height },
              m_super_sample{ super_sample }, m_render_scale{ 0.0f }, m_bake_scale{ 0.0f }, m_line_spacing{ 0.0f },
              m_baseline{ 0.0f }, m_standard_width{ 0.0f } {
            {
                std::ifstream in{ path, std::ios::in | std::ios::binary };
                in.read(reinterpret_cast<char*>(m_font_data.data()), m_font_data.size());
                in.close();
            }
            stbtt_InitFont(&m_font_info, m_font_data.data(), stbtt_GetFontOffsetForIndex(m_font_data.data(), 0));
            m_render_scale = stbtt_ScaleForPixelHeight(&m_font_info, height);
            m_bake_scale = stbtt_ScaleForPixelHeight(&m_font_info, height * super_sample);
            int ascent, descent, line_gap_val;
            stbtt_GetFontVMetrics(&m_font_info, &ascent, &descent, &line_gap_val);
            m_line_spacing = static_cast<float>(ascent - descent + line_gap_val) * m_render_scale;
            m_baseline = static_cast<float>(ascent) * m_render_scale;

            int x0, y0, x1, y1;
            stbtt_GetCodepointBox(&m_font_info, 'W', &x0, &y0, &x1, &y1);
            m_standard_width = static_cast<float>(x1 - x0) * m_render_scale / 1.5f;
        }
        [[nodiscard]] float height() const noexcept override {
            return m_height;
        }
        texture_region render_to_bitmap(const glyph glyph,
                                        const std::function<texture_region(const image_desc&)>& image_uploader) const override {
            int x0, y0, x1, y1;
            stbtt_GetGlyphBitmapBox(&m_font_info, glyph.idx, m_bake_scale, m_bake_scale, &x0, &y0, &x1, &y1);
            const uint32_t w = x1 - x0;
            const uint32_t h = y1 - y0;
            std::pmr::vector<uint8_t> buffer{ static_cast<size_t>(w) * h, m_font_data.get_allocator().resource() };
            const image_desc image{ { w, h }, channel::alpha, buffer.data() };
            stbtt_MakeGlyphBitmap(&m_font_info, const_cast<uint8_t*>(static_cast<const uint8_t*>(buffer.data())), w, h, w,
                                  m_bake_scale, m_bake_scale, glyph.idx);
            return image_uploader(image);
        }
        [[nodiscard]] float calculate_advance(const glyph glyph, const animgui::glyph prev) const override {
            int advance_width, left_side_bearing;
            stbtt_GetGlyphHMetrics(&m_font_info, glyph.idx, &advance_width, &left_side_bearing);
            const auto base = static_cast<float>(advance_width) * m_render_scale;
            if(prev.idx == 0)
                return base;
            return base + static_cast<float>(stbtt_GetGlyphKernAdvance(&m_font_info, prev.idx, glyph.idx)) * m_render_scale;
        }
        [[nodiscard]] float line_spacing() const noexcept override {
            return m_line_spacing;
        }
        [[nodiscard]] glyph to_glyph(const uint32_t codepoint) const override {
            return glyph{ static_cast<uint32_t>(stbtt_FindGlyphIndex(&m_font_info, codepoint)) };
        }
        [[nodiscard]] bounds calculate_bounds(const glyph glyph) const override {
            int x0, y0, x1, y1;
            stbtt_GetGlyphBox(&m_font_info, glyph.idx, &x0, &y0, &x1, &y1);
            return bounds{ static_cast<float>(x0) * m_render_scale, static_cast<float>(x1) * m_render_scale,
                           static_cast<float>(-y1) * m_render_scale + m_baseline,
                           static_cast<float>(-y0) * m_render_scale + m_baseline };
        }
        [[nodiscard]] float standard_width() const noexcept override {
            return m_standard_width;
        }
        [[nodiscard]] float max_scale() const noexcept override {
            return m_super_sample;
        }
    };
    class stb_font_backend final : public font_backend {
        float m_super_sample;

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
        explicit stb_font_backend(const float super_sample) : m_super_sample{ super_sample } {}
        [[nodiscard]] std::shared_ptr<font> load_font(const std::pmr::string& name, float height) const override {
            const auto path = fs::is_regular_file(name) ? fs::path{ name } : locate_font(name);
            if(path.empty())
                throw std::logic_error{ std::string{ "Failed to find font " + name } };
            return std::make_shared<font_impl>(path, height, m_super_sample, name.get_allocator().resource());
        }
    };
    ANIMGUI_API std::shared_ptr<font_backend> create_stb_font_backend(float super_sample) {
        return std::make_shared<stb_font_backend>(super_sample);
    }
}  // namespace animgui
