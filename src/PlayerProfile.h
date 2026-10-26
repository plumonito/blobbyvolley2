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

#include <string>
#include <vector>

/*! \struct PlayerProfile
	\brief a known player: their name and the image of their face
	\details The profiles are listed in players/players.txt, one per line: the name, then
			optionally a semicolon and the file name of the face image in the players folder.
			Empty lines and lines starting with # are ignored.
*/
struct PlayerProfile
{
	std::string name;
	std::string face;

	/// loads players/players.txt; no profiles if the file does not exist
	static std::vector<PlayerProfile> load();
	/// reads the profiles from the content of a players.txt
	static std::vector<PlayerProfile> parse(const std::string& list);
};
