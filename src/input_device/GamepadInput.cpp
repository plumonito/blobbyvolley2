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
#include "InputDevice.h"

/* other includes */
#include "InputManager.h"

// ********************************************************************************************************
// 		Interface Definition
// ********************************************************************************************************

/*! \class GamepadInputDevice
	\brief Ingame input from a gamepad with a standard layout (SDL GameController API)
	\details Move with the d-pad or the left stick, jump with A, B or d-pad up.
*/
class GamepadInputDevice : public InputDevice
{
	public:
		~GamepadInputDevice() override = default;
		GamepadInputDevice(InputManager* inputMgr, int instanceId);

		PlayerInputAbs transferInput() override;

	private:
		int mInstanceId;
};

// ********************************************************************************************************
// 		Class Implementation
// ********************************************************************************************************

// -------------------------------------------------------------------------------------------------
//		Creator Function
// -------------------------------------------------------------------------------------------------

std::unique_ptr<InputDevice> createGamepadInput(InputManager* inputMgr, int instanceId)
{
	return std::unique_ptr<InputDevice>{new GamepadInputDevice(inputMgr, instanceId)};
}

// -------------------------------------------------------------------------------------------------
// 		Gamepad Input Device
// -------------------------------------------------------------------------------------------------

const int GAMEPAD_STICK_THRESHOLD = 12000;

GamepadInputDevice::GamepadInputDevice(InputManager* inputMgr, int instanceId)
	: InputDevice(inputMgr)
	, mInstanceId(instanceId)
{
}

PlayerInputAbs GamepadInputDevice::transferInput()
{
	int stick = mInputManager->getGamepadAxis(mInstanceId, SDL_CONTROLLER_AXIS_LEFTX);

	bool left = stick < -GAMEPAD_STICK_THRESHOLD ||
			mInputManager->isGamepadButtonDown(mInstanceId, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
	bool right = stick > GAMEPAD_STICK_THRESHOLD ||
			mInputManager->isGamepadButtonDown(mInstanceId, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
	bool jump = mInputManager->isGamepadButtonDown(mInstanceId, SDL_CONTROLLER_BUTTON_A) ||
			mInputManager->isGamepadButtonDown(mInstanceId, SDL_CONTROLLER_BUTTON_B) ||
			mInputManager->isGamepadButtonDown(mInstanceId, SDL_CONTROLLER_BUTTON_DPAD_UP);

	return {left, right, jump};
}
