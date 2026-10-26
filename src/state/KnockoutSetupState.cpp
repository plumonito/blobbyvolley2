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
#include "RenderManager.h"
#include "SoundManager.h"
#include "SpeedController.h"
#include "Theme.h"
#include "KnockoutGameState.h"

/* implementation */

namespace
{
	const int MAX_LIVES = 5;
	const int QUIT_CONFIRM_FRAMES = 150;

	const float CARD_WIDTH = 172;
	const float CARD_HEIGHT = 176;
	const float CARD_GAP = 14;
	const float CARDS_X = (800 - 4 * CARD_WIDTH - 3 * CARD_GAP) / 2;
	const float CARDS_Y = 108;
}

KnockoutSetupState::KnockoutSetupState(std::vector<KnockoutPlayer> players, KnockoutSettings settings)
	: mPlayers(std::move(players)), mSettings(settings), mProfiles(PlayerProfile::load())
{
	SpeedController::getMainInstance()->setGameSpeed(75);
}

KnockoutSetupState::~KnockoutSetupState() = default;

void KnockoutSetupState::step_impl()
{
	IMGUI& imgui = getIMGUI();
	InputManager& inputMgr = getInputMgr();

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

	bool livesKey = inputMgr.isKeyPressed(SDLK_l);
	if (livesKey && !mLastLivesKey)
		changeLives();
	mLastLivesKey = livesKey;

	bool drunkKey = inputMgr.isKeyPressed(SDLK_x);
	if (drunkKey && !mLastDrunkKey)
		mSettings.drunk = !mSettings.drunk;
	mLastDrunkKey = drunkKey;

	if (is_exiting())
		mBackRequested = true;

	for (auto& squish : mSquish)
		squish.second.step();
	if (mQuitConfirmFrames > 0)
		--mQuitConfirmFrames;

	// ------------------------------------------------------------------------------------------
	// drawing
	// ------------------------------------------------------------------------------------------
	imgui.doBackground();
	imgui.doPanel(Vector2(0, 0), Vector2(800, 600), theme::PANEL, 0.45, 0);
	imgui.doText(Vector2(400, 10), "Blobby Knockout", TF_HUGE_FONT | TF_ALIGN_CENTER);

	for (int slot = 0; slot < KNOCKOUT_MAX_PLAYERS; ++slot)
	{
		float x = CARDS_X + (slot % 4) * (CARD_WIDTH + CARD_GAP);
		float y = CARDS_Y + (slot / 4) * (CARD_HEIGHT + CARD_GAP);
		drawPlayerCard(slot, x, y);
	}

	// settings: lives as pips, and the drunk mode switch
	{
		const float y = 486;
		const float pipSize = 14;
		const float switchWidth = 44;
		RenderManager& renderer = getRenderer();

		float livesWidth = renderer.getTextWidth("Lives", TF_NORMAL) + 14 + MAX_LIVES * (pipSize + 6) - 6;
		float drunkWidth = renderer.getTextWidth("Drunk mode", TF_NORMAL) + 14 + switchWidth;
		const float padding = 18;
		const float gap = 20;
		float x = 400 - (livesWidth + drunkWidth + 4 * padding + gap) / 2 + padding;

		imgui.doPanel(Vector2(x - padding, y - 6), Vector2(x + livesWidth + padding, y + 38), theme::PANEL, 0.8, 22);
		imgui.doText(Vector2(x, y), "Lives", TF_NORMAL);
		x += renderer.getTextWidth("Lives", TF_NORMAL) + 14;
		for (int i = 1; i <= MAX_LIVES; ++i)
		{
			float top = y + 9;
			if (i <= mSettings.lives)
				imgui.doPanel(Vector2(x, top), Vector2(x + pipSize, top + pipSize), theme::ACCENT, 1, pipSize / 2);
			else
				imgui.doPanel(Vector2(x, top), Vector2(x + pipSize, top + pipSize), theme::TEXT_DIM, 0.3, pipSize / 2);
			x += pipSize + 6;
		}

		x += -6 + padding + gap + padding;
		imgui.doPanel(Vector2(x - padding, y - 6), Vector2(x + drunkWidth + padding, y + 38), theme::PANEL, 0.8, 22);
		imgui.doText(Vector2(x, y), "Drunk mode", TF_NORMAL, mSettings.drunk ? theme::ACCENT : theme::TEXT);
		x += renderer.getTextWidth("Drunk mode", TF_NORMAL) + 14;
		// a switch: the knob is on the right when it is on
		float top = y + 6;
		imgui.doPanel(Vector2(x, top), Vector2(x + switchWidth, top + 22), mSettings.drunk ? theme::ACCENT : theme::TEXT_DIM,
				mSettings.drunk ? 1 : 0.3, 11);
		float knob = mSettings.drunk ? x + switchWidth - 20 : x + 2;
		imgui.doPanel(Vector2(knob, top + 2), Vector2(knob + 18, top + 20), theme::TEXT, 1, 9);
	}

	std::vector<Hint> hints = {
		{"A", theme::BUTTON_A, "Join"},
		{"B", theme::BUTTON_B, "Leave"},
	};
	if (!mProfiles.empty())
		hints.push_back({"<>", theme::BUTTON_MENU, "Name"});
	hints.push_back({"Y", theme::BUTTON_Y, "Lives"});
	hints.push_back({"X", theme::BUTTON_X, "Drunk"});
	hints.push_back({"Start", theme::BUTTON_MENU, "Play", canStart()});
	hints.push_back({"Back", theme::BUTTON_MENU, mQuitConfirmFrames > 0 ? "Again to quit" : "Quit"});
	doHints(536, hints);

	std::string keyboard = "Keyboard:  W/Up join   S/Down leave   ";
	if (!mProfiles.empty())
		keyboard += "A D/Left Right name   ";
	keyboard += "L lives   X drunk   Enter play   Esc quit";
	imgui.doText(Vector2(400, 572), keyboard, TF_SMALL_FONT | TF_ALIGN_CENTER, theme::TEXT_DIM);

	// ------------------------------------------------------------------------------------------
	// state changes
	// ------------------------------------------------------------------------------------------
	if (mBackRequested)
	{
		mBackRequested = false;
		if (mQuitConfirmFrames > 0)
			inputMgr.setEndBlobby();
		else
			mQuitConfirmFrames = QUIT_CONFIRM_FRAMES;
	}
	if (mStartRequested)
	{
		mStartRequested = false;
		if (canStart())
			switchState(new KnockoutGameState(mPlayers, mSettings));
	}
}

void KnockoutSetupState::drawPlayerCard(int slot, float x, float y)
{
	IMGUI& imgui = getIMGUI();
	const float centreX = x + CARD_WIDTH / 2;

	if (slot >= (int)mPlayers.size())
	{
		imgui.doPanel(Vector2(x, y), Vector2(x + CARD_WIDTH, y + CARD_HEIGHT), theme::PANEL, 0.35, 18);
		imgui.doText(Vector2(centreX, y + CARD_HEIGHT / 2 - 22), "Press jump", TF_SMALL_FONT | TF_ALIGN_CENTER, theme::TEXT_DIM);
		imgui.doText(Vector2(centreX, y + CARD_HEIGHT / 2), "to join", TF_SMALL_FONT | TF_ALIGN_CENTER, theme::TEXT_DIM);
		return;
	}

	const KnockoutPlayer& player = mPlayers[slot];
	imgui.doPanel(Vector2(x, y), Vector2(x + CARD_WIDTH, y + CARD_HEIGHT), theme::PANEL, 0.85, 18);

	// order of play
	std::string order = "#" + std::to_string(slot + 1);
	if (slot == 0)
		order += " left";
	else if (slot == 1)
		order += " right";
	imgui.doText(Vector2(x + 12, y + 8), order, TF_SMALL_FONT, theme::ACCENT);

	int skin = getRenderer().getBlobSkin(player.getColor(), player.face);
	imgui.doBlob(Vector2(centreX, y + 76), skin, 1, mSquish[player.number].frame());
	if (!mProfiles.empty())
	{
		imgui.doText(Vector2(x + 16, y + 64), "<", TF_NORMAL, theme::TEXT_DIM);
		imgui.doText(Vector2(x + CARD_WIDTH - 16, y + 64), ">", TF_NORMAL | TF_ALIGN_RIGHT, theme::TEXT_DIM);
	}

	// long names get the smaller font
	std::string name = player.getName();
	unsigned nameFont = getRenderer().getTextWidth(name, TF_NORMAL) <= CARD_WIDTH - 16 ? TF_NORMAL : TF_SMALL_FONT;
	imgui.doText(Vector2(centreX, y + (nameFont == TF_NORMAL ? 122 : 128)), name, nameFont | TF_ALIGN_CENTER);
	imgui.doText(Vector2(centreX, y + 152), player.controller.getName(getInputMgr()), TF_SMALL_FONT | TF_ALIGN_CENTER, theme::TEXT_DIM);
}

void KnockoutSetupState::handleController(const LocalController& controller, const LocalControllerButtons& pressed)
{
	KnockoutPlayer* player = findPlayer(controller);
	if (player == nullptr)
	{
		if (pressed.jump && mPlayers.size() < (unsigned)KNOCKOUT_MAX_PLAYERS)
		{
			KnockoutPlayer newPlayer{controller, KnockoutPlayer::freeNumber(mPlayers), "", ""};
			int profile = findFreeProfile(0, 1);
			if (profile != -1)
			{
				newPlayer.name = mProfiles[profile].name;
				newPlayer.face = mProfiles[profile].face;
			}
			mPlayers.push_back(newPlayer);
			mSquish[newPlayer.number].start();
			playSound(SoundManager::IMPACT, BALL_HIT_PLAYER_SOUND_VOLUME);
		}
	}
	else if (pressed.leave)
	{
		int number = player->number;
		mPlayers.erase(std::remove_if(mPlayers.begin(), mPlayers.end(),
				[&](const KnockoutPlayer& p) { return p.controller == controller; }), mPlayers.end());
		mSquish.erase(number);
	}
	else
	{
		if (pressed.jump)
			mSquish[player->number].start();
		if (pressed.left)
			changeProfile(*player, -1);
		if (pressed.right)
			changeProfile(*player, 1);
	}

	if (pressed.lives)
		changeLives();
	if (pressed.drunk)
		mSettings.drunk = !mSettings.drunk;
	if (pressed.start)
		mStartRequested = true;
	if (pressed.back)
		mBackRequested = true;
}

KnockoutPlayer* KnockoutSetupState::findPlayer(const LocalController& controller)
{
	for (auto& player : mPlayers)
	{
		if (player.controller == controller)
			return &player;
	}
	return nullptr;
}

int KnockoutSetupState::findFreeProfile(int start, int direction) const
{
	int count = mProfiles.size();
	for (int step = 0; step < count; ++step)
	{
		int index = ((start + step * direction) % count + count) % count;
		bool taken = std::any_of(mPlayers.begin(), mPlayers.end(),
				[&](const KnockoutPlayer& player) { return player.name == mProfiles[index].name; });
		if (!taken)
			return index;
	}
	return -1;
}

void KnockoutSetupState::changeProfile(KnockoutPlayer& player, int direction)
{
	if (mProfiles.empty())
		return;

	int current = -1;
	for (unsigned i = 0; i < mProfiles.size(); ++i)
	{
		if (mProfiles[i].name == player.name)
			current = i;
	}

	int next = findFreeProfile(current + direction, direction);
	if (next == -1)
		return;
	player.name = mProfiles[next].name;
	player.face = mProfiles[next].face;
	mSquish[player.number].start();
}

void KnockoutSetupState::changeLives()
{
	mSettings.lives = mSettings.lives % MAX_LIVES + 1;
}

bool KnockoutSetupState::canStart() const
{
	return mPlayers.size() >= 2;
}

const char* KnockoutSetupState::getStateName() const
{
	return "KnockoutSetupState";
}
