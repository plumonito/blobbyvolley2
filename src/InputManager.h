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
#include <utility>
#include <vector>

#include <SDL_keycode.h>
#include <SDL_gamecontroller.h>

#include "BlobbyDebug.h"

class BlobbyApp;

/// \brief class for managing input: the keyboard and the gamepads
class InputManager : public ObjectCounter<InputManager>
{
	public:
		explicit InputManager(BlobbyApp* app);
		~InputManager();

		/// processes the events of this frame, call once per frame
		void updateInput();

		bool running() const;
		void setEndBlobby();	// call to trigger the event that ends blobby, i.e. running will return false after this call.

		/// true in the frame in which escape was pressed
		bool exit() const;

		bool isKeyPressed(SDL_Keycode code) const;

		// Gamepads (SDL GameController API), identified by their SDL joystick instance id.
		// They are opened automatically when connected and listed in connection order.
		std::vector<int> getGamepads() const;
		bool isGamepadButtonDown(int instanceId, SDL_GameControllerButton button) const;
		int getGamepadAxis(int instanceId, SDL_GameControllerAxis axis) const;
		/// lets the gamepad vibrate, if it supports that. strength is between 0 and 1.
		void rumbleGamepad(int instanceId, float strength, int durationMs);

	private:
		SDL_GameController* findGamepad(int instanceId) const;

		bool mRunning = true;
		bool mExit = false;

		BlobbyApp* mApp;
		std::vector<std::pair<int, SDL_GameController*>> mGamepads;
};
