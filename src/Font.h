/*=============================================================================
Blobby Volley 2
Copyright (C) 2026 Blobby Volley 2 contributors

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "stb_truetype.h"

/*! \class Font
	\brief a TrueType font, rendered with stb_truetype
*/
class Font
{
	public:
		/// loads the font file through the virtual file system
		explicit Font(const std::string& filename);

		/// a rendered line of text: the coverage of every pixel, 0 to 255
		struct Bitmap
		{
			int width = 0;
			int height = 0;
			std::vector<std::uint8_t> coverage;
		};

		/// renders a line of UTF-8 text. The bitmap is as high as the line of the font
		/// at the given pixel size, and as wide as the text.
		Bitmap render(const std::string& text, float pixelHeight) const;
		/// width of a line of UTF-8 text in pixels
		int measure(const std::string& text, float pixelHeight) const;

		/// decodes UTF-8 into code points; invalid bytes become '?'
		static std::vector<int> decodeUTF8(const std::string& text);

	private:
		std::vector<unsigned char> mData;
		stbtt_fontinfo mInfo;
};
