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
#include "PlayerProfile.h"

/* includes */
#include <sstream>

#include "FileRead.h"

/* implementation */

namespace
{
	const char PROFILE_LIST[] = "players/players.txt";
}

std::vector<PlayerProfile> PlayerProfile::load()
{
	try
	{
		FileRead file(PROFILE_LIST);
		auto content = file.readRawBytes(file.length());
		return parse(std::string(content.begin(), content.end()));
	}
	catch (const std::exception&)
	{
		return {};
	}
}

std::vector<PlayerProfile> PlayerProfile::parse(const std::string& list)
{
	auto trim = [](std::string text) {
		const char* space = " \t\r\n";
		text.erase(0, text.find_first_not_of(space));
		text.erase(text.find_last_not_of(space) + 1);
		return text;
	};

	std::vector<PlayerProfile> profiles;
	std::istringstream lines(list);
	std::string line;
	while (std::getline(lines, line))
	{
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;

		std::size_t separator = line.find(';');
		PlayerProfile profile;
		profile.name = trim(line.substr(0, separator));
		if (separator != std::string::npos)
			profile.face = trim(line.substr(separator + 1));
		if (!profile.name.empty())
			profiles.push_back(profile);
	}
	return profiles;
}
