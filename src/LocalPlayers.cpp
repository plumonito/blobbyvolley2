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
#include "LocalPlayers.h"

/* includes */
#include <algorithm>
#include <cassert>

#include <SDL.h>

#include "InputDevice.h"
#include "InputManager.h"

/* implementation */

namespace
{
	struct KeyboardLayout
	{
		const char* name;
		SDL_Keycode left;
		SDL_Keycode right;
		SDL_Keycode jump;
		SDL_Keycode leave;
	};

	const KeyboardLayout KEYBOARD_LAYOUTS[] = {
		{"keys wasd", SDLK_a, SDLK_d, SDLK_w, SDLK_s},
		{"keys arrows", SDLK_LEFT, SDLK_RIGHT, SDLK_UP, SDLK_DOWN},
	};

	const int KEYBOARD_LAYOUT_COUNT = sizeof(KEYBOARD_LAYOUTS) / sizeof(KEYBOARD_LAYOUTS[0]);

	const Color PLAYER_COLORS[KNOCKOUT_MAX_PLAYERS] = {
		Color(0, 0, 255),		// blue
		Color(255, 0, 0),		// red
		Color(0, 200, 0),		// green
		Color(255, 220, 0),		// yellow
		Color(160, 0, 220),		// purple
		Color(255, 128, 0),		// orange
		Color(0, 220, 220),		// cyan
		Color(255, 100, 200),	// pink
	};
}

std::vector<LocalController> LocalController::getAvailable(const InputManager& inputMgr)
{
	std::vector<LocalController> controllers;
	for (int i = 0; i < KEYBOARD_LAYOUT_COUNT; ++i)
		controllers.push_back(LocalController{KEYBOARD, i});
	for (int pad : inputMgr.getGamepads())
		controllers.push_back(LocalController{GAMEPAD, pad});
	return controllers;
}

std::string LocalController::getName(const InputManager& inputMgr) const
{
	if (type == KEYBOARD)
		return KEYBOARD_LAYOUTS[id].name;

	// the SDL gamepad names contain characters our font cannot display, so we number the pads instead
	auto pads = inputMgr.getGamepads();
	auto pos = std::find(pads.begin(), pads.end(), id);
	if (pos == pads.end())
		return "pad (disconnected)";
	return "pad " + std::to_string(pos - pads.begin() + 1);
}

std::unique_ptr<InputDevice> LocalController::createInputDevice(InputManager* inputMgr) const
{
	if (type == KEYBOARD)
	{
		const KeyboardLayout& layout = KEYBOARD_LAYOUTS[id];
		return createKeyboardInput(inputMgr, layout.left, layout.right, layout.jump);
	}
	return createGamepadInput(inputMgr, id);
}

LocalControllerButtons LocalControllerButtons::poll(const LocalController& controller, const InputManager& inputMgr)
{
	LocalControllerButtons buttons;
	if (controller.type == LocalController::KEYBOARD)
	{
		const KeyboardLayout& layout = KEYBOARD_LAYOUTS[controller.id];
		buttons.jump = inputMgr.isKeyPressed(layout.jump);
		buttons.leave = inputMgr.isKeyPressed(layout.leave);
	}
	else
	{
		int pad = controller.id;
		buttons.jump = inputMgr.isGamepadButtonDown(pad, SDL_CONTROLLER_BUTTON_A);
		buttons.leave = inputMgr.isGamepadButtonDown(pad, SDL_CONTROLLER_BUTTON_B);
		buttons.lives = inputMgr.isGamepadButtonDown(pad, SDL_CONTROLLER_BUTTON_Y);
		buttons.start = inputMgr.isGamepadButtonDown(pad, SDL_CONTROLLER_BUTTON_START);
		buttons.back = inputMgr.isGamepadButtonDown(pad, SDL_CONTROLLER_BUTTON_BACK);
	}
	return buttons;
}

LocalControllerButtons LocalControllerButtons::pressedSince(const LocalControllerButtons& before) const
{
	LocalControllerButtons pressed;
	pressed.jump = jump && !before.jump;
	pressed.leave = leave && !before.leave;
	pressed.lives = lives && !before.lives;
	pressed.start = start && !before.start;
	pressed.back = back && !before.back;
	return pressed;
}

std::string KnockoutPlayer::getName() const
{
	return "player " + std::to_string(number);
}

Color KnockoutPlayer::getColor() const
{
	assert(number >= 1 && number <= KNOCKOUT_MAX_PLAYERS);
	return PLAYER_COLORS[number - 1];
}

int KnockoutPlayer::freeNumber(const std::vector<KnockoutPlayer>& players)
{
	for (int number = 1; number <= KNOCKOUT_MAX_PLAYERS; ++number)
	{
		bool used = std::any_of(players.begin(), players.end(),
				[&](const KnockoutPlayer& player) { return player.number == number; });
		if (!used)
			return number;
	}
	return 0;
}
