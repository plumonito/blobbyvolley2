/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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
#include "RenderManager.h"

/* includes */
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "stb_image.h"

#include "DuelMatchState.h"
#include "FileRead.h"
#include "FileExceptions.h"
#include "Font.h"
#include "Theme.h"

/* implementation */

namespace
{
	const char FONT_FILE[] = "fonts/Fredoka-SemiBold.ttf";
	const int BLOB_WIDTH = 75;
	const int BLOB_HEIGHT = 89;
	const int BALL_SIZE = 64;
	/// text textures not used for this many frames are thrown away
	const unsigned TEXT_CACHE_FRAMES = 300;

	float fontSize(unsigned int flags)
	{
		if (flags & TF_HUGE_FONT)
			return 72;
		if (flags & TF_LARGE_FONT)
			return 44;
		if (flags & TF_SMALL_FONT)
			return 18;
		return 28;
	}

	/// blends a colour with straight alpha over another one, both channels 0 to 1
	void blendOver(float src[4], const float dst[4])
	{
		float alpha = src[3] + dst[3] * (1 - src[3]);
		for (int c = 0; c < 3; ++c)
			src[c] = alpha > 0 ? (src[c] * src[3] + dst[c] * dst[3] * (1 - src[3])) / alpha : 0;
		src[3] = alpha;
	}

	float smoothstep(float edge0, float edge1, float x)
	{
		float t = std::max(0.f, std::min(1.f, (x - edge0) / (edge1 - edge0)));
		return t * t * (3 - 2 * t);
	}
}

RenderManager::RenderManager() = default;

RenderManager::~RenderManager()
{
	for (auto& skin : mSkins)
	{
		for (auto texture : skin.frames)
			SDL_DestroyTexture(texture);
	}
	for (auto& text : mTextCache)
		SDL_DestroyTexture(text.second.texture);
	for (auto& image : mImages)
		SDL_DestroyTexture(image.second);
	for (auto texture : mBall)
		SDL_DestroyTexture(texture);
	for (auto surface : mBlobFrames)
		SDL_FreeSurface(surface);

	SDL_DestroyTexture(mBackground);
	SDL_DestroyTexture(mWhite);
	SDL_DestroyTexture(mCircle);
	SDL_DestroyTexture(mRenderTarget);
	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
}

void RenderManager::init(int xResolution, int yResolution, bool fullscreen)
{
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	Uint32 screenFlags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE;
	mWindow = SDL_CreateWindow(AppTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			xResolution, yResolution, screenFlags);

	SDL_Surface* icon = loadSurface("Icon.bmp");
	SDL_SetColorKey(icon, SDL_TRUE, SDL_MapRGB(icon->format, 0, 0, 0));
	SDL_SetWindowIcon(mWindow, icon);
	SDL_FreeSurface(icon);

	mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
	SDL_ShowCursor(0);

	// draw everything in the base resolution, refresh scales it to the window
	mRenderTarget = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, xResolution, yResolution);
	SDL_SetRenderTarget(mRenderer, mRenderTarget);
	SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);

	// a white pixel for rectangles and a white disc for rounded corners, both tinted when drawn
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 1, 1, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_FillRect(surface, nullptr, 0xFFFFFFFF);
	mWhite = SDL_CreateTextureFromSurface(mRenderer, surface);
	SDL_FreeSurface(surface);

	const int circleSize = 64;
	surface = SDL_CreateRGBSurfaceWithFormat(0, circleSize, circleSize, 32, SDL_PIXELFORMAT_ARGB8888);
	Uint32* pixels = (Uint32*)surface->pixels;
	for (int y = 0; y < circleSize; ++y)
	{
		for (int x = 0; x < circleSize; ++x)
		{
			float dx = x + 0.5f - circleSize / 2.f;
			float dy = y + 0.5f - circleSize / 2.f;
			float coverage = std::max(0.f, std::min(1.f, circleSize / 2.f - std::sqrt(dx * dx + dy * dy) + 0.5f));
			pixels[y * surface->pitch / 4 + x] = ((Uint32)lround(coverage * 255) << 24) | 0xFFFFFF;
		}
	}
	mCircle = SDL_CreateTextureFromSurface(mRenderer, surface);
	SDL_SetTextureBlendMode(mCircle, SDL_BLENDMODE_BLEND);
	SDL_FreeSurface(surface);

	setBackground("backgrounds/strand2.bmp");

	for (int i = 1; i <= 16; ++i)
	{
		char filename[64];
		snprintf(filename, sizeof(filename), "gfx/ball%02d.bmp", i);
		surface = loadSurface(filename);
		SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0, 0));
		mBall.push_back(SDL_CreateTextureFromSurface(mRenderer, surface));
		SDL_FreeSurface(surface);
	}

	// the blob animation frames: black is transparent
	for (int i = 1; i <= 5; ++i)
	{
		char filename[64];
		snprintf(filename, sizeof(filename), "gfx/blobbym%d.bmp", i);
		SDL_Surface* image = loadSurface(filename);
		SDL_Surface* blob = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ABGR8888, 0);
		SDL_FreeSurface(image);
		for (int p = 0; p < blob->w * blob->h; ++p)
		{
			SDL_Color* pixel = &((SDL_Color*)blob->pixels)[p];
			pixel->a = (pixel->r | pixel->g | pixel->b) ? 255 : 0;
		}
		mBlobFrames.push_back(blob);
	}

	mFont.reset(new Font(FONT_FILE));
}

SDL_Window* RenderManager::getWindow()
{
	return mWindow;
}

void RenderManager::setTitle(const std::string& title)
{
	SDL_SetWindowTitle(mWindow, title.c_str());
}

bool RenderManager::setBackground(const std::string& filename)
{
	try
	{
		SDL_Surface* surface = loadSurface(filename);
		SDL_DestroyTexture(mBackground);
		mBackground = SDL_CreateTextureFromSurface(mRenderer, surface);
		SDL_FreeSurface(surface);
	}
	catch (const FileLoadException&)
	{
		return false;
	}
	return true;
}

SDL_Surface* RenderManager::loadSurface(const std::string& filename)
{
	FileRead file(filename);
	auto content = file.readRawBytes(file.length());

	SDL_RWops* rwops = SDL_RWFromMem(content.data(), content.size());
	SDL_Surface* surface = SDL_LoadBMP_RW(rwops, 1);
	if (!surface)
		BOOST_THROW_EXCEPTION(FileLoadException(filename));
	return surface;
}

// -------------------------------------------------------------------------------------------------
//		Blobs
// -------------------------------------------------------------------------------------------------

int RenderManager::getBlobSkin(Color color, const std::string& faceImage)
{
	for (unsigned i = 0; i < mSkins.size(); ++i)
	{
		if (mSkins[i].color == color && mSkins[i].face == faceImage)
			return i;
	}

	// the face is cut to a square from its centre
	std::vector<std::uint8_t> face;
	int faceSize = 0;
	if (!faceImage.empty())
	{
		try
		{
			FileRead file("players/" + faceImage);
			auto content = file.readRawBytes(file.length());
			int width, height, channels;
			stbi_uc* pixels = stbi_load_from_memory((const stbi_uc*)content.data(), content.size(), &width, &height, &channels, 4);
			if (pixels)
			{
				faceSize = std::min(width, height);
				int x0 = (width - faceSize) / 2;
				int y0 = (height - faceSize) / 2;
				face.resize(faceSize * faceSize * 4);
				for (int y = 0; y < faceSize; ++y)
					std::copy_n(pixels + ((y0 + y) * width + x0) * 4, faceSize * 4, face.begin() + y * faceSize * 4);
				stbi_image_free(pixels);
			}
			else
			{
				std::cerr << "Warning: could not read face image " << faceImage << ": " << stbi_failure_reason() << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Warning: could not load face image " << faceImage << ": " << e.what() << std::endl;
		}
	}

	BlobSkin skin{color, faceImage, {}};
	for (auto frame : mBlobFrames)
	{
		SDL_Surface* surface = createSkinFrame(frame, color, face, faceSize);
		SDL_Texture* texture = SDL_CreateTextureFromSurface(mRenderer, surface);
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_FreeSurface(surface);
		skin.frames.push_back(texture);
	}
	mSkins.push_back(skin);
	return mSkins.size() - 1;
}

SDL_Surface* RenderManager::createSkinFrame(SDL_Surface* blob, Color color, const std::vector<std::uint8_t>& face, int faceSize)
{
	SDL_Surface* result = SDL_ConvertSurfaceFormat(blob, SDL_PIXELFORMAT_ABGR8888, 0);
	SDL_Color* pixels = (SDL_Color*)result->pixels;
	const int width = result->w;
	const int height = result->h;

	// outline of the blob in this squish frame, and its average brightness
	int minX = width, maxX = -1, minY = height, maxY = -1;
	float brightnessSum = 0;
	int count = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const SDL_Color& pixel = pixels[y * width + x];
			if (pixel.a == 0)
				continue;
			minX = std::min(minX, x); maxX = std::max(maxX, x);
			minY = std::min(minY, y); maxY = std::max(maxY, y);
			brightnessSum += (pixel.r + pixel.g + pixel.b) / (3 * 255.f);
			++count;
		}
	}
	const float averageBrightness = count ? brightnessSum / count : 1;
	const float centreX = (minX + maxX + 1) / 2.f;
	const float centreY = (minY + maxY + 1) / 2.f;
	// the face covers most of the blob, the rest keeps the colour of the player
	const float radiusX = (maxX - minX + 1) / 2.f * 0.9f;
	const float radiusY = (maxY - minY + 1) / 2.f * 0.9f;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			SDL_Color& pixel = pixels[y * width + x];
			if (pixel.a == 0)
				continue;

			// colour the white blob like the original game: multiply, but keep bright highlights bright
			int highlight = int(pixel.r) * 5 - 4 * 256 - 138;
			auto tint = [highlight](int channel, int tintChannel) {
				int value = (channel * tintChannel) >> 8;
				if (highlight > 0)
					value += highlight;
				return std::max(1, std::min(value, 255));
			};
			float out[3] = { (float)tint(pixel.r, color.r), (float)tint(pixel.g, color.g), (float)tint(pixel.b, color.b) };

			if (faceSize > 0)
			{
				// wrap the face around the blob like around a ball: towards the edges it is squeezed
				float nx = (x + 0.5f - centreX) / radiusX;
				float ny = (y + 0.5f - centreY) / radiusY;
				float distance = std::sqrt(nx * nx + ny * ny);
				if (distance < 1)
				{
					float u = 0.5f + std::asin(std::max(-1.f, std::min(1.f, nx))) / float(M_PI);
					float v = 0.5f + std::asin(std::max(-1.f, std::min(1.f, ny))) / float(M_PI);

					// bilinear sample
					float fx = u * faceSize - 0.5f;
					float fy = v * faceSize - 0.5f;
					int ix = std::max(0, std::min(faceSize - 2, (int)std::floor(fx)));
					int iy = std::max(0, std::min(faceSize - 2, (int)std::floor(fy)));
					float ax = std::max(0.f, std::min(1.f, fx - ix));
					float ay = std::max(0.f, std::min(1.f, fy - iy));
					float sample[4];
					for (int c = 0; c < 4; ++c)
					{
						auto at = [&](int sx, int sy) { return (float)face[(sy * faceSize + sx) * 4 + c]; };
						sample[c] = (at(ix, iy) * (1 - ax) + at(ix + 1, iy) * ax) * (1 - ay)
								  + (at(ix, iy + 1) * (1 - ax) + at(ix + 1, iy + 1) * ax) * ay;
					}

					// light the face like the blob: darker where the blob is darker, plus its highlights
					float brightness = (pixel.r + pixel.g + pixel.b) / (3 * 255.f);
					float shade = std::max(0.45f, std::min(1.35f, brightness / averageBrightness));
					float shine = std::max(0.f, (brightness - 0.88f) / 0.12f) * 0.7f;
					float mask = smoothstep(1.f, 0.88f, distance) * sample[3] / 255;
					for (int c = 0; c < 3; ++c)
					{
						float lit = std::min(255.f, sample[c] * shade);
						lit += (255 - lit) * shine;
						out[c] = out[c] * (1 - mask) + lit * mask;
					}
				}
			}

			pixel.r = (Uint8)lround(out[0]);
			pixel.g = (Uint8)lround(out[1]);
			pixel.b = (Uint8)lround(out[2]);
		}
	}
	return result;
}

void RenderManager::setBlobSkin(PlayerSide side, int skin)
{
	mCourtSkin[side] = skin;
}

void RenderManager::setWaitingBlobs(std::vector<WaitingBlob> blobs)
{
	mWaitingBlobs = std::move(blobs);
}

void RenderManager::drawBlob(Vector2 position, int skin, float scale, int frame)
{
	if (skin < 0 || skin >= (int)mSkins.size())
		return;

	int width = (int)lround(BLOB_WIDTH * scale);
	int height = (int)lround(BLOB_HEIGHT * scale);
	SDL_Rect rect = { (int)lround(position.x - width / 2.f), (int)lround(position.y - height / 2.f), width, height };
	SDL_RenderCopy(mRenderer, mSkins[skin].frames[std::max(0, std::min(4, frame))], nullptr, &rect);
}

void RenderManager::drawBackground()
{
	SDL_RenderCopy(mRenderer, mBackground, nullptr, nullptr);
}

void RenderManager::drawGame(const DuelMatchState& gameState)
{
	drawBackground();

	// the waiting players stand behind everything else
	for (const auto& blob : mWaitingBlobs)
		drawBlob(blob.position, blob.skin, blob.scale, blob.frame);

	// draw the net rod over them again
	SDL_Rect rod = { 400 - 7, 300, 14, 300 };
	SDL_RenderCopy(mRenderer, mBackground, &rod, &rod);

	// marker for a ball that is above the screen
	Vector2 ball = gameState.getBallPosition();
	if (ball.y < 0)
	{
		SDL_Rect marker = { (int)lround(ball.x) - 4, 4, 8, 8 };
		SDL_SetTextureColorMod(mWhite, 255, 255, 255);
		SDL_SetTextureAlphaMod(mWhite, SDL_GetTicks() % 1000 >= 500 ? 255 : 120);
		SDL_RenderCopy(mRenderer, mWhite, nullptr, &marker);
	}

	SDL_Rect ballRect = { (int)lround(ball.x) - BALL_SIZE / 2, (int)lround(ball.y) - BALL_SIZE / 2, BALL_SIZE, BALL_SIZE };
	int ballFrame = int(gameState.getBallRotation() / M_PI / 2 * 16) % 16;
	SDL_RenderCopy(mRenderer, mBall[ballFrame], nullptr, &ballRect);

	for (PlayerSide side : {LEFT_PLAYER, RIGHT_PLAYER})
		drawBlob(gameState.getBlobPosition(side), mCourtSkin[side], 1, int(gameState.getBlobState(side)) % 5);
}

// -------------------------------------------------------------------------------------------------
//		User interface
// -------------------------------------------------------------------------------------------------

const RenderManager::TextTexture& RenderManager::getTextTexture(const std::string& text, unsigned int flags, Color color)
{
	unsigned sizeFlags = flags & (TF_SMALL_FONT | TF_LARGE_FONT | TF_HUGE_FONT);
	std::string key = text + '\x1f' + std::to_string(sizeFlags) + '\x1f' + std::to_string(color.toInt());

	auto cached = mTextCache.find(key);
	if (cached != mTextCache.end())
	{
		cached->second.lastUsed = mFrame;
		return cached->second;
	}

	const float size = fontSize(flags);
	Font::Bitmap glyphs = mFont->render(text, size);

	// a dark outline around the letters and a soft shadow below them
	const int outline = std::max(2, (int)lround(size / 11));
	const int shadow = std::max(1, (int)lround(size / 14));
	const int width = glyphs.width + 2 * outline;
	const int height = glyphs.height + 2 * outline + shadow;

	auto coverage = [&](int x, int y) -> float {
		x -= outline; y -= outline;
		if (x < 0 || y < 0 || x >= glyphs.width || y >= glyphs.height)
			return 0;
		return glyphs.coverage[y * glyphs.width + x] / 255.f;
	};

	std::vector<float> outlined(width * height, 0);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			float strongest = 0;
			for (int dy = -outline; dy <= outline && strongest < 1; ++dy)
			{
				for (int dx = -outline; dx <= outline; ++dx)
				{
					float distance = std::sqrt(float(dx * dx + dy * dy));
					if (distance > outline + 0.5f)
						continue;
					float weight = std::min(1.f, outline + 0.5f - distance);
					strongest = std::max(strongest, coverage(x + dx, y + dy) * weight);
				}
			}
			outlined[y * width + x] = strongest;
		}
	}

	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
	Uint32* pixels = (Uint32*)surface->pixels;
	const Color edge = theme::TEXT_OUTLINE;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			float pixel[4] = { 0, 0, 0, y >= shadow ? outlined[(y - shadow) * width + x] * 0.35f : 0 };
			float border[4] = { edge.r / 255.f, edge.g / 255.f, edge.b / 255.f, outlined[y * width + x] };
			float fill[4] = { color.r / 255.f, color.g / 255.f, color.b / 255.f, coverage(x, y) };
			blendOver(border, pixel);
			blendOver(fill, border);

			Uint32 a = (Uint32)lround(fill[3] * 255);
			Uint32 r = (Uint32)lround(fill[0] * 255);
			Uint32 g = (Uint32)lround(fill[1] * 255);
			Uint32 b = (Uint32)lround(fill[2] * 255);
			pixels[y * surface->pitch / 4 + x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}

	TextTexture entry{SDL_CreateTextureFromSurface(mRenderer, surface), width, height, outline, mFrame};
	SDL_SetTextureBlendMode(entry.texture, SDL_BLENDMODE_BLEND);
	SDL_FreeSurface(surface);
	return mTextCache.emplace(key, entry).first->second;
}

int RenderManager::getTextWidth(const std::string& text, unsigned int flags)
{
	return mFont->measure(text, fontSize(flags));
}

void RenderManager::drawText(const std::string& text, Vector2 position, unsigned int flags, Color color)
{
	if (text.empty())
		return;

	const TextTexture& texture = getTextTexture(text, flags, color);
	float textWidth = texture.width - 2 * texture.padding;
	float x = position.x;
	if (flags & TF_ALIGN_CENTER)
		x -= textWidth / 2;
	else if (flags & TF_ALIGN_RIGHT)
		x -= textWidth;

	SDL_Rect rect = { (int)lround(x) - texture.padding, (int)lround(position.y) - texture.padding, texture.width, texture.height };
	SDL_RenderCopy(mRenderer, texture.texture, nullptr, &rect);
}

void RenderManager::drawImage(const std::string& filename, Vector2 position, Vector2 size)
{
	SDL_Texture*& texture = mImages[filename];
	if (!texture)
	{
		SDL_Surface* surface = loadSurface(filename);
		SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0, 0, 0));
		texture = SDL_CreateTextureFromSurface(mRenderer, surface);
		SDL_FreeSurface(surface);
	}

	int width, height;
	SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
	if (size != Vector2(0, 0))
	{
		width = (int)lround(size.x);
		height = (int)lround(size.y);
	}
	SDL_Rect rect = { (int)lround(position.x - width / 2.f), (int)lround(position.y - height / 2.f), width, height };
	SDL_RenderCopy(mRenderer, texture, nullptr, &rect);
}

void RenderManager::drawPanel(Vector2 pos1, Vector2 pos2, Color color, float alpha, float radius)
{
	int x = (int)lround(pos1.x);
	int y = (int)lround(pos1.y);
	int w = (int)lround(pos2.x - pos1.x);
	int h = (int)lround(pos2.y - pos1.y);
	int r = std::max(0, std::min({ (int)lround(radius), w / 2, h / 2 }));
	Uint8 a = (Uint8)lround(std::max(0.f, std::min(1.f, alpha)) * 255);

	for (SDL_Texture* texture : { mWhite, mCircle })
	{
		SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
		SDL_SetTextureAlphaMod(texture, a);
	}

	// the middle column and the two side columns between the corners do not overlap
	SDL_Rect middle = { x + r, y, w - 2 * r, h };
	SDL_Rect left = { x, y + r, r, h - 2 * r };
	SDL_Rect right = { x + w - r, y + r, r, h - 2 * r };
	SDL_RenderCopy(mRenderer, mWhite, nullptr, &middle);
	if (r == 0)
		return;
	SDL_RenderCopy(mRenderer, mWhite, nullptr, &left);
	SDL_RenderCopy(mRenderer, mWhite, nullptr, &right);

	const int half = 32;
	SDL_Rect corners[4][2] = {
		{ { 0, 0, half, half }, { x, y, r, r } },
		{ { half, 0, half, half }, { x + w - r, y, r, r } },
		{ { 0, half, half, half }, { x, y + h - r, r, r } },
		{ { half, half, half, half }, { x + w - r, y + h - r, r, r } },
	};
	for (auto& corner : corners)
		SDL_RenderCopy(mRenderer, mCircle, &corner[0], &corner[1]);
}

void RenderManager::refresh()
{
	SDL_SetRenderTarget(mRenderer, nullptr);

	// the window may have been resized
	SDL_Rect viewport;
	int windowX, windowY;
	SDL_RenderGetViewport(mRenderer, &viewport);
	SDL_GetWindowSize(mWindow, &windowX, &windowY);
	if (viewport.w != windowX || viewport.h != windowY)
	{
		viewport.w = windowX;
		viewport.h = windowY;
		SDL_RenderSetViewport(mRenderer, &viewport);
	}

	SDL_RenderCopy(mRenderer, mRenderTarget, nullptr, nullptr);
	SDL_RenderPresent(mRenderer);
	SDL_SetRenderTarget(mRenderer, mRenderTarget);

	// forget text that is not shown any more, like old clock times
	++mFrame;
	for (auto it = mTextCache.begin(); it != mTextCache.end(); )
	{
		if (mFrame - it->second.lastUsed > TEXT_CACHE_FRAMES)
		{
			SDL_DestroyTexture(it->second.texture);
			it = mTextCache.erase(it);
		}
		else
		{
			++it;
		}
	}
}
