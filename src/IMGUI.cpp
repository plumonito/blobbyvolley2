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
#include "IMGUI.h"

/* implementation */

void IMGUI::begin()
{
	mItems.clear();
}

void IMGUI::end(RenderManager& renderer)
{
	for (const auto& item : mItems)
	{
		switch (item.type)
		{
			case BACKGROUND:
				renderer.drawBackground();
				break;
			case IMAGE:
				renderer.drawImage(item.text, item.pos1, item.pos2);
				break;
			case TEXT:
				renderer.drawText(item.text, item.pos1, item.flags, item.color);
				break;
			case PANEL:
				renderer.drawPanel(item.pos1, item.pos2, item.color, item.value, item.radius);
				break;
			case BLOB:
				renderer.drawBlob(item.pos1, item.skin, item.value, item.frame);
				break;
		}
	}
	mItems.clear();
}

void IMGUI::doBackground()
{
	mItems.push_back(Item{BACKGROUND, Vector2(), Vector2(), "", 0, Color(), 0, 0, 0, 0});
}

void IMGUI::doImage(const Vector2& position, const std::string& name, const Vector2& size)
{
	mItems.push_back(Item{IMAGE, position, size, name, 0, Color(), 0, 0, 0, 0});
}

void IMGUI::doText(const Vector2& position, const std::string& text, unsigned int flags, Color color)
{
	mItems.push_back(Item{TEXT, position, Vector2(), text, flags, color, 0, 0, 0, 0});
}

void IMGUI::doPanel(const Vector2& pos1, const Vector2& pos2, Color color, float alpha, float radius)
{
	mItems.push_back(Item{PANEL, pos1, pos2, "", 0, color, alpha, radius, 0, 0});
}

void IMGUI::doBlob(const Vector2& position, int skin, float scale, int frame)
{
	mItems.push_back(Item{BLOB, position, Vector2(), "", 0, Color(), scale, 0, skin, frame});
}
