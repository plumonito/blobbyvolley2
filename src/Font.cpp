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

/* header include */
#include "Font.h"

/* includes */
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "FileRead.h"

/* implementation */

Font::Font(const std::string& filename)
{
	FileRead file(filename);
	auto bytes = file.readRawBytes(file.length());
	mData.assign(bytes.begin(), bytes.end());

	if (!stbtt_InitFont(&mInfo, mData.data(), stbtt_GetFontOffsetForIndex(mData.data(), 0)))
		throw std::runtime_error("could not load font " + filename);
}

std::vector<int> Font::decodeUTF8(const std::string& text)
{
	std::vector<int> codepoints;
	for (std::size_t i = 0; i < text.size(); )
	{
		unsigned char c = text[i];
		int length = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
		if (length == 0 || i + length > text.size())
		{
			codepoints.push_back('?');
			++i;
			continue;
		}

		int codepoint = length == 1 ? c : c & (0xFF >> (length + 1));
		for (int k = 1; k < length; ++k)
			codepoint = (codepoint << 6) | (text[i + k] & 0x3F);
		codepoints.push_back(codepoint);
		i += length;
	}
	return codepoints;
}

int Font::measure(const std::string& text, float pixelHeight) const
{
	float scale = stbtt_ScaleForPixelHeight(&mInfo, pixelHeight);
	auto codepoints = decodeUTF8(text);

	float x = 0;
	for (std::size_t i = 0; i < codepoints.size(); ++i)
	{
		int advance, leftBearing;
		stbtt_GetCodepointHMetrics(&mInfo, codepoints[i], &advance, &leftBearing);
		x += advance * scale;
		if (i + 1 < codepoints.size())
			x += scale * stbtt_GetCodepointKernAdvance(&mInfo, codepoints[i], codepoints[i + 1]);
	}
	return (int)std::ceil(x);
}

Font::Bitmap Font::render(const std::string& text, float pixelHeight) const
{
	float scale = stbtt_ScaleForPixelHeight(&mInfo, pixelHeight);
	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&mInfo, &ascent, &descent, &lineGap);
	int baseline = (int)std::ceil(ascent * scale);

	Bitmap bitmap;
	bitmap.width = std::max(1, measure(text, pixelHeight) + 2);
	bitmap.height = baseline + (int)std::ceil(-descent * scale);
	bitmap.coverage.assign(bitmap.width * bitmap.height, 0);

	auto codepoints = decodeUTF8(text);
	std::vector<unsigned char> glyph;
	float x = 1;
	for (std::size_t i = 0; i < codepoints.size(); ++i)
	{
		int codepoint = codepoints[i];
		float shift = x - std::floor(x);

		int x0, y0, x1, y1;
		stbtt_GetCodepointBitmapBoxSubpixel(&mInfo, codepoint, scale, scale, shift, 0, &x0, &y0, &x1, &y1);
		int width = x1 - x0;
		int height = y1 - y0;
		if (width > 0 && height > 0)
		{
			glyph.assign(width * height, 0);
			stbtt_MakeCodepointBitmapSubpixel(&mInfo, glyph.data(), width, height, width, scale, scale, shift, 0, codepoint);

			// glyphs may overlap a little, so keep the stronger coverage
			for (int gy = 0; gy < height; ++gy)
			{
				int py = baseline + y0 + gy;
				if (py < 0 || py >= bitmap.height)
					continue;
				for (int gx = 0; gx < width; ++gx)
				{
					int px = (int)std::floor(x) + x0 + gx;
					if (px < 0 || px >= bitmap.width)
						continue;
					auto& target = bitmap.coverage[py * bitmap.width + px];
					target = std::max(target, glyph[gy * width + gx]);
				}
			}
		}

		int advance, leftBearing;
		stbtt_GetCodepointHMetrics(&mInfo, codepoint, &advance, &leftBearing);
		x += advance * scale;
		if (i + 1 < codepoints.size())
			x += scale * stbtt_GetCodepointKernAdvance(&mInfo, codepoint, codepoints[i + 1]);
	}
	return bitmap;
}
