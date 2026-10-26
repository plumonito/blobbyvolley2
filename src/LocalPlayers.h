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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Color.h"

class InputManager;
class InputDevice;

/// maximum number of local players in a knockout match
const int KNOCKOUT_MAX_PLAYERS = 8;

/*! \struct LocalController
	\brief a local controller a player can use: one of the keyboard layouts, or a gamepad
*/
struct LocalController
{
	enum Type
	{
		KEYBOARD,
		GAMEPAD
	};

	Type type;
	int id;		///< keyboard layout index, or SDL joystick instance id of the gamepad

	bool operator==(const LocalController& other) const { return type == other.type && id == other.id; }
	bool operator!=(const LocalController& other) const { return !(*this == other); }

	/// all controllers that are currently available: the keyboard layouts followed by the connected gamepads
	static std::vector<LocalController> getAvailable(const InputManager& inputMgr);
	/// short display name, e.g. "keys wasd" or "pad 2"
	std::string getName(const InputManager& inputMgr) const;
	/// creates the in-game input device of this controller
	std::unique_ptr<InputDevice> createInputDevice(InputManager* inputMgr) const;
};

/*! \struct LocalControllerButtons
	\brief the state of the buttons of a controller that are relevant for the setup menu
*/
struct LocalControllerButtons
{
	bool jump = false;
	bool leave = false;	///< keyboard: down, gamepad: B
	bool lives = false;	///< change the number of lives, gamepad: Y
	bool start = false;	///< gamepad: start
	bool back = false;	///< gamepad: back / select

	static LocalControllerButtons poll(const LocalController& controller, const InputManager& inputMgr);
	/// the buttons that are held now but were not held before
	LocalControllerButtons pressedSince(const LocalControllerButtons& before) const;
};

/*! \class LocalControllerWatcher
	\brief detects which buttons the available controllers press from frame to frame
*/
class LocalControllerWatcher
{
	public:
		using Presses = std::vector<std::pair<LocalController, LocalControllerButtons>>;

		/// polls all available controllers, call once per frame.
		/// \return the buttons each controller pressed since the last call. A controller that
		///			is seen for the first time reports no presses, so a button that is still held
		///			(e.g. from selecting the current menu) does not count.
		Presses poll(const InputManager& inputMgr);

	private:
		Presses mLastButtons;
};

/*! \struct KnockoutPlayer
	\brief a player taking part in a knockout match
*/
struct KnockoutPlayer
{
	LocalController controller;
	int number;	///< player number 1..KNOCKOUT_MAX_PLAYERS, determines name and colour

	std::string getName() const;
	Color getColor() const;

	/// the lowest player number that is not used by any of the players, or 0 if all are taken
	static int freeNumber(const std::vector<KnockoutPlayer>& players);
};
