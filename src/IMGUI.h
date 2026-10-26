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

#include <string>
#include <vector>

#include "Vector.h"
#include "Color.h"
#include "RenderManager.h"
#include "Theme.h"
#include "BlobbyDebug.h"

/*! \class IMGUI
	\brief collects the user interface of a frame and draws it on top of the game
	\details The states describe their interface every frame; end() draws it in that order.
*/
class IMGUI : public ObjectCounter<IMGUI>
{
	public:
		void begin();
		void end(RenderManager& renderer);

		/// the background of the beach
		void doBackground();
		void doImage(const Vector2& position, const std::string& name, const Vector2& size = Vector2(0, 0));
		/// a line of text, see RenderManager::drawText
		void doText(const Vector2& position, const std::string& text, unsigned int flags = TF_NORMAL, Color color = theme::TEXT);
		/// a rectangle with rounded corners
		void doPanel(const Vector2& pos1, const Vector2& pos2, Color color = theme::PANEL, float alpha = 0.8, float radius = 16);
		/// a blob, centred at position
		void doBlob(const Vector2& position, int skin, float scale = 1, int frame = 0);

	private:
		enum Type
		{
			BACKGROUND,
			IMAGE,
			TEXT,
			PANEL,
			BLOB
		};

		struct Item
		{
			Type type;
			Vector2 pos1;
			Vector2 pos2;
			std::string text;
			unsigned int flags;
			Color color;
			float value;	// alpha of a panel, scale of a blob
			float radius;
			int skin;
			int frame;
		};

		std::vector<Item> mItems;
};
