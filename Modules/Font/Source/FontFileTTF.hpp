/*!
* This file is a portion of Luna SDK.
* For conditions of distribution and use, see the disclaimer
* and license in LICENSE.txt
* 
* @file FontFileTTF.hpp
* @author JXMaster
* @date 2019/10/7
*/
#pragma once
#include "FontHeader.hpp"
#include "StbTrueType.hpp"
#include <Runtime/Vector.hpp>

namespace Luna
{
	namespace Font
	{
		struct FontFileTTF : IFontFile
		{
			lustruct("Font::FontFileTTF", "{5ead091d-2a74-49b8-9201-f97b7d19685f}");
			luiimpl();

			Blob m_data;
			Vector<stbtt_fontinfo> m_infos;

			FontFileTTF() {}

			RV init(const byte_t* data, usize data_size);
			const Blob& data()
			{
				return m_data;
			}
			u32 count_fonts();
			glyph_t find_glyph(u32 font_index, u32 codepoint);
			f32 scale_for_pixel_height(u32 font_index, f32 pixels);
			void get_vmetrics(u32 font_index, i32* ascent, i32* descent, i32* lineGap);
			void get_glyph_hmetrics(u32 font_index, glyph_t glyph, i32 *advance_width, i32 *left_side_bearing);
			i32 get_kern_advance(u32 font_index, glyph_t ch1, glyph_t ch2);
			Vector<i16> get_glyph_shape(u32 font_index, glyph_t glyph);
			RectI get_glyph_bounding_box(u32 font_index, glyph_t glyph);
			RectI get_glyph_bitmap_box(u32 font_index, glyph_t glyph, f32 scale_x, f32 scale_y, f32 shift_x, f32 shift_y);
			void render_glyph_bitmap(u32 font_index, glyph_t glyph, void* output, i32 out_w, i32 out_h, i32 out_row_pitch,
				f32 scale_x, f32 scale_y, f32 shift_x, f32 shift_y);
		};
	}
}