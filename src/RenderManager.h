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

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL.h>

#include "Vector.h"
#include "Color.h"
#include "Global.h"
#include "BlobbyDebug.h"

class Font;
struct DuelMatchState;

// Text flags: size and alignment
static const int TF_NORMAL		= 0x00;
static const int TF_SMALL_FONT	= 0x02;
static const int TF_LARGE_FONT	= 0x04;
static const int TF_HUGE_FONT	= 0x20;

static const int TF_ALIGN_LEFT	= 0x00;	// Text aligned to the left (default)
static const int TF_ALIGN_CENTER= 0x08;	// Text centered
static const int TF_ALIGN_RIGHT = 0x10;	// Text aligned right

/*! \class RenderManager
	\brief draws the game and the user interface with SDL
	\details Everything is drawn into a 800x600 render target, which is scaled to the window.
*/
class RenderManager : public ObjectCounter<RenderManager>
{
	public:
		RenderManager();
		~RenderManager();

		void init(int xResolution, int yResolution, bool fullscreen);
		/// shows what was drawn, call after all draw calls of a frame
		void refresh();

		SDL_Window* getWindow();
		void setTitle(const std::string& title);
		bool setBackground(const std::string& filename);

		/// the look of a blob: its colour and optionally a face image (in the players folder),
		/// which is wrapped around it. Returns an id for setBlobSkin and drawBlob.
		int getBlobSkin(Color color, const std::string& faceImage = "");
		/// the look of the blob on a side of the court
		void setBlobSkin(PlayerSide side, int skin);

		/// players waiting for their turn: drawGame draws them behind everything else, in this order
		struct WaitingBlob
		{
			Vector2 position;	// centre
			float scale;		// size relative to a blob on court
			int skin;
			int frame;			// squish animation frame, 0 to 4
		};
		void setWaitingBlobs(std::vector<WaitingBlob> blobs);

		void drawGame(const DuelMatchState& gameState);
		void drawBackground();

		/// draws a line of text. position is the top left corner of the line, or the top centre /
		/// top right corner when aligned. The text gets an outline and a soft shadow.
		void drawText(const std::string& text, Vector2 position, unsigned int flags, Color color);
		int getTextWidth(const std::string& text, unsigned int flags);
		/// draws a BMP image (black is transparent) centred at position, optionally scaled
		void drawImage(const std::string& filename, Vector2 position, Vector2 size = Vector2(0, 0));
		/// draws a rectangle with rounded corners
		void drawPanel(Vector2 pos1, Vector2 pos2, Color color, float alpha, float radius);
		/// draws a blob, centred at position
		void drawBlob(Vector2 position, int skin, float scale, int frame);

	private:
		struct BlobSkin
		{
			Color color;
			std::string face;
			std::vector<SDL_Texture*> frames;
		};

		struct TextTexture
		{
			SDL_Texture* texture;
			int width;
			int height;
			int padding;		// outline around the text
			unsigned lastUsed;	// frame number
		};

		SDL_Surface* loadSurface(const std::string& filename);
		SDL_Surface* createSkinFrame(SDL_Surface* blob, Color color, const std::vector<std::uint8_t>& face, int faceSize);
		const TextTexture& getTextTexture(const std::string& text, unsigned int flags, Color color);

		SDL_Window* mWindow = nullptr;
		SDL_Renderer* mRenderer = nullptr;
		// render target in the base resolution, scaled to the window
		SDL_Texture* mRenderTarget = nullptr;

		SDL_Texture* mBackground = nullptr;
		SDL_Texture* mWhite = nullptr;
		SDL_Texture* mCircle = nullptr;
		std::vector<SDL_Texture*> mBall;
		/// the blob animation frames in white, to be coloured
		std::vector<SDL_Surface*> mBlobFrames;
		std::vector<BlobSkin> mSkins;
		int mCourtSkin[MAX_PLAYERS] = {-1, -1};
		std::vector<WaitingBlob> mWaitingBlobs;

		std::map<std::string, SDL_Texture*> mImages;

		std::unique_ptr<Font> mFont;
		std::unordered_map<std::string, TextTexture> mTextCache;
		unsigned mFrame = 0;
};
