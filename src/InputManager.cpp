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
#include "InputManager.h"

/* includes */
#include <SDL.h>

/* implementation */

InputManager::InputManager(BlobbyApp* app) : mApp(app)
{
	// gamepads: SDL reports already connected controllers as SDL_CONTROLLERDEVICEADDED events
	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
	SDL_GameControllerEventState(SDL_ENABLE);
}

InputManager::~InputManager()
{
	for (auto& pad : mGamepads)
		SDL_GameControllerClose(pad.second);
}

void InputManager::updateInput()
{
	mExit = false;

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				mRunning = false;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE && !event.key.repeat)
					mExit = true;
				break;
			case SDL_CONTROLLERDEVICEADDED:
			{
				SDL_GameController* pad = SDL_GameControllerOpen(event.cdevice.which);
				if (pad != nullptr)
				{
					int instanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
					if (findGamepad(instanceId) == nullptr)
						mGamepads.emplace_back(instanceId, pad);
					else
						SDL_GameControllerClose(pad);	// already open, drop the extra reference
				}
				break;
			}
			case SDL_CONTROLLERDEVICEREMOVED:
				for (auto it = mGamepads.begin(); it != mGamepads.end(); ++it)
				{
					if (it->first == event.cdevice.which)
					{
						SDL_GameControllerClose(it->second);
						mGamepads.erase(it);
						break;
					}
				}
				break;
			default:
				break;
		}
	}
}

bool InputManager::running() const
{
	return mRunning;
}

void InputManager::setEndBlobby()
{
	mRunning = false;
}

bool InputManager::exit() const
{
	return mExit;
}

bool InputManager::isKeyPressed(SDL_Keycode code) const
{
	const Uint8* keyState = SDL_GetKeyboardState(nullptr);
	return keyState[SDL_GetScancodeFromKey(code)];
}

std::vector<int> InputManager::getGamepads() const
{
	std::vector<int> ids;
	for (const auto& pad : mGamepads)
		ids.push_back(pad.first);
	return ids;
}

SDL_GameController* InputManager::findGamepad(int instanceId) const
{
	for (const auto& pad : mGamepads)
	{
		if (pad.first == instanceId)
			return pad.second;
	}
	return nullptr;
}

bool InputManager::isGamepadButtonDown(int instanceId, SDL_GameControllerButton button) const
{
	SDL_GameController* pad = findGamepad(instanceId);
	return pad != nullptr && SDL_GameControllerGetButton(pad, button) != 0;
}

int InputManager::getGamepadAxis(int instanceId, SDL_GameControllerAxis axis) const
{
	SDL_GameController* pad = findGamepad(instanceId);
	return pad ? SDL_GameControllerGetAxis(pad, axis) : 0;
}

void InputManager::rumbleGamepad(int instanceId, float strength, int durationMs)
{
	SDL_GameController* pad = findGamepad(instanceId);
	if (pad == nullptr)
		return;

	Uint16 intensity = (Uint16)(strength * 0xFFFF);
	// fails silently for gamepads without rumble motors
	SDL_GameControllerRumble(pad, intensity, intensity, durationMs);
}
