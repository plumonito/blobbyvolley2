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
#include "KnockoutSetupState.h"

/* includes */
#include <algorithm>

#include <SDL.h>

#include "IMGUI.h"
#include "InputManager.h"
#include "SoundManager.h"
#include "KnockoutGameState.h"

/* implementation */

namespace
{
	const int MAX_LIVES = 5;
	const unsigned ROWS_PER_COLUMN = KNOCKOUT_MAX_PLAYERS / 2;
	const float COLUMN_X[2] = {40, 420};
	const float ROW_HEIGHT = 62;
	const unsigned MAX_SMALL_LINE_LENGTH = 62;
}

KnockoutSetupState::KnockoutSetupState(std::vector<KnockoutPlayer> players, int lives)
	: mPlayers(std::move(players)), mLives(lives)
{
}

KnockoutSetupState::~KnockoutSetupState() = default;

void KnockoutSetupState::step_impl()
{
	IMGUI& imgui = getIMGUI();
	InputManager& inputMgr = getInputMgr();

	// the controllers navigate this menu themselves, so the keyboard / joystick selection of
	// the buttons is disabled: otherwise pressing jump could click a highlighted button
	imgui.resetSelection();

	// ------------------------------------------------------------------------------------------
	// input
	// ------------------------------------------------------------------------------------------
	auto controllers = LocalController::getAvailable(inputMgr);

	// players whose gamepad was disconnected leave the match
	mPlayers.erase(std::remove_if(mPlayers.begin(), mPlayers.end(), [&](const KnockoutPlayer& player) {
			return std::find(controllers.begin(), controllers.end(), player.controller) == controllers.end();
		}), mPlayers.end());

	for (const auto& press : mControllers.poll(inputMgr))
		handleController(press.first, press.second);

	bool returnHeld = inputMgr.isKeyPressed(SDLK_RETURN) || inputMgr.isKeyPressed(SDLK_KP_ENTER);
	if (returnHeld && !mLastReturn)
		mStartRequested = true;
	mLastReturn = returnHeld;

	if (is_exiting())
		mBackRequested = true;

	// ------------------------------------------------------------------------------------------
	// drawing
	// ------------------------------------------------------------------------------------------
	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doText(GEN_ID, Vector2(400, 18), "knockout", TF_ALIGN_CENTER);

	imgui.doOverlay(GEN_ID, Vector2(20, 58), Vector2(780, 318));
	for (unsigned i = 0; i < mPlayers.size(); ++i)
	{
		const float x = COLUMN_X[i / ROWS_PER_COLUMN];
		const float y = 72 + (i % ROWS_PER_COLUMN) * ROW_HEIGHT;

		std::string controller = mPlayers[i].controller.getName(inputMgr);
		if (i == 0)
			controller += " - starts left";
		else if (i == 1)
			controller += " - starts right";

		imgui.doOverlay(GEN_ID, Vector2(x, y), Vector2(x + 24, y + 24), mPlayers[i].getColor(), 1.0);
		imgui.doText(GEN_ID, Vector2(x + 36, y), mPlayers[i].getName());
		imgui.doText(GEN_ID, Vector2(x + 36, y + 28), controller, TF_SMALL_FONT);
	}
	if (mPlayers.empty())
		imgui.doText(GEN_ID, Vector2(400, 176), "nobody has joined yet", TF_ALIGN_CENTER);

	if (imgui.doButton(GEN_ID, Vector2(400, 334), "lives: " + std::to_string(mLives), TF_ALIGN_CENTER))
		changeLives();

	// controllers that have not joined yet
	std::vector<std::string> waitingLines;
	std::string line;
	for (const auto& controller : controllers)
	{
		if (hasJoined(controller))
			continue;

		std::string name = controller.getName(inputMgr);
		if (line.empty())
			line = name;
		else if (line.size() + 2 + name.size() <= MAX_SMALL_LINE_LENGTH)
			line += ", " + name;
		else
		{
			waitingLines.push_back(line);
			line = name;
		}
	}
	if (!line.empty())
		waitingLines.push_back(line);

	if (mPlayers.size() >= (unsigned)KNOCKOUT_MAX_PLAYERS)
	{
		imgui.doText(GEN_ID, Vector2(400, 380), "the match is full", TF_SMALL_FONT | TF_ALIGN_CENTER);
	}
	else if (!waitingLines.empty())
	{
		imgui.doText(GEN_ID, Vector2(400, 380), "press jump to join:", TF_SMALL_FONT | TF_ALIGN_CENTER);
		for (unsigned i = 0; i < waitingLines.size() && i < 2; ++i)
			imgui.doText(GEN_ID, Vector2(400, 396 + i * 16), waitingLines[i], TF_SMALL_FONT | TF_ALIGN_CENTER);
	}

	imgui.doText(GEN_ID, Vector2(400, 446), "touch the ball and the next in line takes over.", TF_SMALL_FONT | TF_ALIGN_CENTER);
	imgui.doText(GEN_ID, Vector2(400, 462), "miss and you lose a life. last one standing wins.", TF_SMALL_FONT | TF_ALIGN_CENTER);
	imgui.doText(GEN_ID, Vector2(400, 494), "jump: join   down / b: leave   y: lives", TF_SMALL_FONT | TF_ALIGN_CENTER);
	imgui.doText(GEN_ID, Vector2(400, 510), "start / enter: play   back / esc: menu", TF_SMALL_FONT | TF_ALIGN_CENTER);

	if (imgui.doButton(GEN_ID, Vector2(34, 562), TextManager::LBL_CANCEL))
		mBackRequested = true;

	if (canStart())
	{
		if (imgui.doButton(GEN_ID, Vector2(766, 562), TextManager::MNU_LABEL_START, TF_ALIGN_RIGHT))
			mStartRequested = true;
	}
	else
	{
		imgui.doText(GEN_ID, Vector2(766, 568), "needs two players", TF_SMALL_FONT | TF_ALIGN_RIGHT);
	}

	// ------------------------------------------------------------------------------------------
	// state changes
	// ------------------------------------------------------------------------------------------
	if (mBackRequested)
	{
		switchState(new MainMenuState());
	}
	else if (mStartRequested)
	{
		mStartRequested = false;
		if (canStart())
			switchState(new KnockoutGameState(mPlayers, mLives));
	}
}

void KnockoutSetupState::handleController(const LocalController& controller, const LocalControllerButtons& pressed)
{
	if (!hasJoined(controller))
	{
		if (pressed.jump && mPlayers.size() < (unsigned)KNOCKOUT_MAX_PLAYERS)
		{
			mPlayers.push_back(KnockoutPlayer{controller, KnockoutPlayer::freeNumber(mPlayers)});
			playSound(SoundManager::IMPACT, BALL_HIT_PLAYER_SOUND_VOLUME);
		}
	}
	else if (pressed.leave)
	{
		leave(controller);
	}

	if (pressed.lives)
		changeLives();
	if (pressed.start)
		mStartRequested = true;
	if (pressed.back)
		mBackRequested = true;
}

bool KnockoutSetupState::hasJoined(const LocalController& controller) const
{
	return std::any_of(mPlayers.begin(), mPlayers.end(),
			[&](const KnockoutPlayer& player) { return player.controller == controller; });
}

void KnockoutSetupState::leave(const LocalController& controller)
{
	mPlayers.erase(std::remove_if(mPlayers.begin(), mPlayers.end(),
			[&](const KnockoutPlayer& player) { return player.controller == controller; }),
			mPlayers.end());
}

void KnockoutSetupState::changeLives()
{
	mLives = mLives % MAX_LIVES + 1;
}

bool KnockoutSetupState::canStart() const
{
	return mPlayers.size() >= 2;
}

const char* KnockoutSetupState::getStateName() const
{
	return "KnockoutSetupState";
}
