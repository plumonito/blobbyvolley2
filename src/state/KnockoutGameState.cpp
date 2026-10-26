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
#include "KnockoutGameState.h"

/* includes */
#include <algorithm>

#include <SDL.h>

#include "BlobbyApp.h"
#include "DuelMatch.h"
#include "DuelMatchState.h"
#include "GameConstants.h"
#include "IMGUI.h"
#include "InputDevice.h"
#include "InputManager.h"
#include "IUserConfigReader.h"
#include "KnockoutSetupState.h"
#include "LocalInputSource.h"
#include "MatchEvents.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "SpeedController.h"
#include "SwitchableInputSource.h"
#include "Theme.h"

/* implementation */

namespace
{
	/// the knockout rules decide the winner, so the rule script must never end the match
	const int KNOCKOUT_SCORE_TO_WIN = 999999;
	/// how long "Anna is out!" is shown
	const int MESSAGE_FRAMES = 150;
	/// vibration of the gamepad of a player who has just taken over a blob
	const float TURN_RUMBLE_STRENGTH = 0.75;
	const int TURN_RUMBLE_MS = 300;
	/// confirms that a player joined
	const float JOIN_RUMBLE_STRENGTH = 0.4;
	const int JOIN_RUMBLE_MS = 150;
}

KnockoutGameState::KnockoutGameState(std::vector<KnockoutPlayer> players, int lives)
	: mPlayers(std::move(players))
	, mLives(lives)
	, mQueue(mPlayers.size(), lives)
{
}

KnockoutGameState::~KnockoutGameState() = default;

void KnockoutGameState::init()
{
	std::shared_ptr<IUserConfigReader> config = IUserConfigReader::createUserConfigReader("config.xml");
	SpeedController::getMainInstance()->setGameSpeed( (float)config->getInteger("gamefps") );

	playSound(SoundManager::WHISTLE, ROUND_START_SOUND_VOLUME);

	mMatch.reset(new DuelMatch( false, config->getString("rules"), KNOCKOUT_SCORE_TO_WIN ));

	for (const auto& player : mPlayers)
	{
		mPlayerInputs.push_back(std::make_shared<LocalInputSource>(player.controller.createInputDevice(&getInputMgr())));
		mSkins.push_back(getRenderer().getBlobSkin(player.getColor(), player.face));
	}

	mCourtInputs[LEFT_PLAYER] = std::make_shared<SwitchableInputSource>();
	mCourtInputs[RIGHT_PLAYER] = std::make_shared<SwitchableInputSource>();
	mMatch->setInputSources(mCourtInputs[LEFT_PLAYER], mCourtInputs[RIGHT_PLAYER]);
	showPlayersOnCourt();
	mWaiting.setQueue(mQueue.getWaiting());
}

void KnockoutGameState::step_impl()
{
	auto presses = mControllers.poll(getInputMgr());
	bool startPressed = false, backPressed = false, leavePressed = false;
	for (const auto& press : presses)
	{
		startPressed = startPressed || press.second.start;
		backPressed = backPressed || press.second.back;
		leavePressed = leavePressed || (press.second.leave && press.first.type == LocalController::GAMEPAD);
	}
	bool returnHeld = getInputMgr().isKeyPressed(SDLK_RETURN) || getInputMgr().isKeyPressed(SDLK_KP_ENTER);
	bool returnPressed = returnHeld && !mLastReturn;
	mLastReturn = returnHeld;
	bool escapePressed = is_exiting();

	if (mWinnerSide != NO_PLAYER)
	{
		if (startPressed || returnPressed)
			switchState(new KnockoutGameState(mPlayers, mLives));
		else if (leavePressed || backPressed || escapePressed)
			switchState(new KnockoutSetupState(mPlayers, mLives));

		++mWinnerFrames;
		if (mWinnerFrames % 60 == 0)
			mWinnerSquish.start();
		mWinnerSquish.step();
	}
	else if (mPaused)
	{
		if (startPressed || returnPressed)
		{
			mPaused = false;
			mMatch->unpause();
		}
		else if (backPressed || escapePressed)
		{
			switchState(new KnockoutSetupState(mPlayers, mLives));
		}
	}
	else if (startPressed || escapePressed)
	{
		mPaused = true;
		mMatch->pause();
	}
	else
	{
		handleJoins(presses);

		mMatch->step();
		updateKnockout();
		stepWaitingPlayers();
		playSounds();

		int winner = mQueue.getWinner();
		if (winner != -1)
			mWinnerSide = mQueue.getActive(LEFT_PLAYER) == winner ? LEFT_PLAYER : RIGHT_PLAYER;

		if (mMessageFrames > 0)
			--mMessageFrames;
	}

	presentMatch();
	if (mWinnerSide != NO_PLAYER)
		presentWinner();
	else if (mPaused)
		presentPause();
}

// -------------------------------------------------------------------------------------------------
//		Rules
// -------------------------------------------------------------------------------------------------

void KnockoutGameState::updateKnockout()
{
	bool touched[MAX_PLAYERS];
	bool error[MAX_PLAYERS] = {false, false};
	bool ballReset = false;

	for (PlayerSide side : {LEFT_PLAYER, RIGHT_PLAYER})
	{
		// DuelMatch counts every valid touch, including the faulty fourth one, for which the
		// touch counter of the rules is already reset again
		int touchCount = mMatch->getTouchCount(side);
		touched[side] = touchCount > mLastTouchCount[side];
		mLastTouchCount[side] = touchCount;
	}

	for (const auto& event : mMatch->getEvents())
	{
		if (event.event == MatchEvent::PLAYER_ERROR && event.side != NO_PLAYER)
			error[event.side] = true;
		if (event.event == MatchEvent::RESET_BALL)
			ballReset = true;
	}

	bool turnOver[MAX_PLAYERS];
	bool rallyRunning = !mMatch->getBallDown() && !ballReset;
	mTurns.update(touched, mMatch->getBallPosition().x, rallyRunning, turnOver);

	const int onCourt[MAX_PLAYERS] = {mQueue.getActive(LEFT_PLAYER), mQueue.getActive(RIGHT_PLAYER)};
	if (mQueue.update(turnOver, error))
	{
		// players who leave the court walk back to the queue from where their blob was
		for (PlayerSide side : {LEFT_PLAYER, RIGHT_PLAYER})
		{
			int player = onCourt[side];
			if (mQueue.getActive(side) != player && !mQueue.isKnockedOut(player))
				mWaiting.arrive(player, mMatch->getBlobPosition(side).x);
		}

		for (int player : mQueue.getKnockedOut())
			showMessage(mPlayers[player].getName() + " is out!");

		showPlayersOnCourt();
	}

	// also picks up players who just joined
	mWaiting.setQueue(mQueue.getWaiting());
}

void KnockoutGameState::stepWaitingPlayers()
{
	mWaiting.step([&](int player) {
		return mPlayerInputs[player]->updateInput().toPlayerInput(mMatch.get());
	});
}

void KnockoutGameState::handleJoins(const LocalControllerWatcher::Presses& presses)
{
	for (const auto& press : presses)
	{
		const LocalController& controller = press.first;
		if (!press.second.jump || hasJoined(controller))
			continue;

		// every player of a match keeps their colour, so at most eight can ever take part
		int number = KnockoutPlayer::freeNumber(mPlayers);
		if (number == 0 || mQueue.addPlayer(mLives) == -1)
			continue;

		// latecomers get the first profile nobody plays with
		KnockoutPlayer player{controller, number, "", ""};
		for (const auto& profile : PlayerProfile::load())
		{
			bool taken = std::any_of(mPlayers.begin(), mPlayers.end(),
					[&](const KnockoutPlayer& other) { return other.name == profile.name; });
			if (!taken)
			{
				player.name = profile.name;
				player.face = profile.face;
				break;
			}
		}

		mPlayers.push_back(player);
		mPlayerInputs.push_back(std::make_shared<LocalInputSource>(controller.createInputDevice(&getInputMgr())));
		mSkins.push_back(getRenderer().getBlobSkin(player.getColor(), player.face));

		showMessage(player.getName() + " joined");
		playSound(SoundManager::IMPACT, BALL_HIT_PLAYER_SOUND_VOLUME);
		if (controller.type == LocalController::GAMEPAD)
			getInputMgr().rumbleGamepad(controller.id, JOIN_RUMBLE_STRENGTH, JOIN_RUMBLE_MS);
	}
}

bool KnockoutGameState::hasJoined(const LocalController& controller) const
{
	return std::any_of(mPlayers.begin(), mPlayers.end(),
			[&](const KnockoutPlayer& player) { return player.controller == controller; });
}

void KnockoutGameState::showPlayersOnCourt()
{
	for (PlayerSide side : {LEFT_PLAYER, RIGHT_PLAYER})
	{
		int active = mQueue.getActive(side);
		mCourtInputs[side]->setSource(mPlayerInputs[active]);
		getRenderer().setBlobSkin(side, mSkins[active]);

		if (active != mAnnouncedOnCourt[side] && mQueue.getWinner() == -1)
		{
			announceTurn(active);
			mAnnouncedOnCourt[side] = active;
		}
	}
}

void KnockoutGameState::announceTurn(int player)
{
	const LocalController& controller = mPlayers[player].controller;
	if (controller.type == LocalController::GAMEPAD)
		getInputMgr().rumbleGamepad(controller.id, TURN_RUMBLE_STRENGTH, TURN_RUMBLE_MS);
}

void KnockoutGameState::showMessage(const std::string& text)
{
	mMessage = text;
	mMessageFrames = MESSAGE_FRAMES;
}

void KnockoutGameState::playSounds()
{
	for (const auto& event : mMatch->getEvents())
	{
		if (event.event == MatchEvent::BALL_HIT_BLOB)
			playSound(SoundManager::IMPACT, event.intensity + BALL_HIT_PLAYER_SOUND_VOLUME);
		if (event.event == MatchEvent::PLAYER_ERROR)
			playSound(SoundManager::WHISTLE, ROUND_START_SOUND_VOLUME);
	}
}

// -------------------------------------------------------------------------------------------------
//		Drawing
// -------------------------------------------------------------------------------------------------

void KnockoutGameState::presentMatch()
{
	const bool running = mWinnerSide == NO_PLAYER;

	// the waiting players walk around at the back of the beach, the next one in front
	std::vector<WaitingPlayers::View> waiting;
	std::vector<RenderManager::WaitingBlob> waitingBlobs;
	if (running)
	{
		waiting = mWaiting.getViews();
		for (const auto& view : waiting)
			waitingBlobs.push_back(RenderManager::WaitingBlob{view.position, view.scale, mSkins[view.player], view.frame});
	}

	RenderManager& renderer = getRenderer();
	renderer.setWaitingBlobs(waitingBlobs);
	renderer.drawGame(mMatch->getState());
	renderer.setWaitingBlobs({});

	IMGUI& imgui = getIMGUI();

	// names over the waiting players
	for (const auto& view : waiting)
	{
		Vector2 top = view.position - Vector2(0, BLOBBY_HEIGHT * view.scale / 2 + 22);
		imgui.doText(top, mPlayers[view.player].getName(), TF_SMALL_FONT | TF_ALIGN_CENTER);
		if (view.player == mQueue.getWaiting().front())
		{
			float width = renderer.getTextWidth("Next", TF_SMALL_FONT) + 16;
			imgui.doPanel(top - Vector2(width / 2, 26), top + Vector2(width / 2, -2), theme::ACCENT, 1, 12);
			imgui.doText(top - Vector2(0, 25), "Next", TF_SMALL_FONT | TF_ALIGN_CENTER);
		}
	}

	// players left and time
	std::string left = std::to_string(mQueue.getRemaining()) + " left";
	float leftWidth = renderer.getTextWidth(left, TF_NORMAL);
	imgui.doPanel(Vector2(12, 12), Vector2(12 + leftWidth + 28, 52), theme::PANEL, 0.75, 20);
	imgui.doText(Vector2(26, 16), left, TF_NORMAL);

	std::string time = mMatch->getTimeString();
	float timeWidth = renderer.getTextWidth(time, TF_SMALL_FONT);
	imgui.doPanel(Vector2(400 - timeWidth / 2 - 14, 12), Vector2(400 + timeWidth / 2 + 14, 42), theme::PANEL, 0.75, 15);
	imgui.doText(Vector2(400, 16), time, TF_SMALL_FONT | TF_ALIGN_CENTER);

	if (!running)
		return;

	if (KnockoutPlayer::freeNumber(mPlayers) != 0)
		imgui.doText(Vector2(400, 48), "Press jump to join", TF_SMALL_FONT | TF_ALIGN_CENTER, theme::TEXT_DIM);

	doNameTag(LEFT_PLAYER);
	doNameTag(RIGHT_PLAYER);

	if (mMessageFrames > 0)
	{
		float width = renderer.getTextWidth(mMessage, TF_LARGE_FONT);
		imgui.doPanel(Vector2(400 - width / 2 - 26, 94), Vector2(400 + width / 2 + 26, 160), theme::PANEL, 0.8, 33);
		imgui.doText(Vector2(400, 100), mMessage, TF_LARGE_FONT | TF_ALIGN_CENTER);
	}
}

void KnockoutGameState::doNameTag(PlayerSide side)
{
	IMGUI& imgui = getIMGUI();
	int player = mQueue.getActive(side);
	std::string name = mPlayers[player].getName();

	const float dot = 16;
	const float pip = 10;
	const float height = 40;
	float width = 14 + dot + 10 + getRenderer().getTextWidth(name, TF_NORMAL) + 14;
	if (mLives > 1)
		width += 4 + mLives * (pip + 4);

	float x = side == LEFT_PLAYER ? 12 : 788 - width;
	const float y = 600 - 12 - height;
	imgui.doPanel(Vector2(x, y), Vector2(x + width, y + height), theme::PANEL, 0.75, height / 2);
	x += 14;
	imgui.doPanel(Vector2(x, y + 12), Vector2(x + dot, y + 12 + dot), mPlayers[player].getColor(), 1, dot / 2);
	x += dot + 10;
	imgui.doText(Vector2(x, y + 4), name, TF_NORMAL);
	x += getRenderer().getTextWidth(name, TF_NORMAL) + 4;

	if (mLives > 1)
	{
		for (int i = 1; i <= mLives; ++i)
		{
			float top = y + (height - pip) / 2;
			bool alive = i <= mQueue.getLives(player);
			imgui.doPanel(Vector2(x, top), Vector2(x + pip, top + pip), alive ? theme::ACCENT : theme::TEXT_DIM, alive ? 1 : 0.3, pip / 2);
			x += pip + 4;
		}
	}
}

void KnockoutGameState::presentPause()
{
	IMGUI& imgui = getIMGUI();
	imgui.doPanel(Vector2(0, 0), Vector2(800, 600), theme::PANEL, 0.5, 0);
	imgui.doPanel(Vector2(190, 200), Vector2(610, 380), theme::PANEL, 0.92, 28);
	imgui.doText(Vector2(400, 222), "Paused", TF_LARGE_FONT | TF_ALIGN_CENTER);
	doHints(296, { {"Start", theme::BUTTON_MENU, "Resume"}, {"Back", theme::BUTTON_MENU, "End match"} });
	imgui.doText(Vector2(400, 340), "Keyboard:  Enter resume   Esc end match", TF_SMALL_FONT | TF_ALIGN_CENTER, theme::TEXT_DIM);
}

void KnockoutGameState::presentWinner()
{
	IMGUI& imgui = getIMGUI();
	const KnockoutPlayer& winner = mPlayers[mQueue.getWinner()];

	imgui.doPanel(Vector2(0, 0), Vector2(800, 600), theme::PANEL, 0.5, 0);
	imgui.doPanel(Vector2(140, 100), Vector2(660, 500), theme::PANEL, 0.92, 32);

	std::string title = winner.getName() + " wins!";
	unsigned font = getRenderer().getTextWidth(title, TF_HUGE_FONT) <= 480 ? TF_HUGE_FONT : TF_LARGE_FONT;
	imgui.doText(Vector2(400, font == TF_HUGE_FONT ? 114 : 128), title, font | TF_ALIGN_CENTER, theme::ACCENT);

	// the trophy is 130x360
	imgui.doImage(Vector2(310, 300), "gfx/pokal.bmp", Vector2(63, 175));
	imgui.doBlob(Vector2(490, 304), mSkins[mQueue.getWinner()], 1.6, mWinnerSquish.frame());

	imgui.doText(Vector2(400, 392), "Last blob standing", TF_NORMAL | TF_ALIGN_CENTER, theme::TEXT_DIM);
	doHints(434, { {"Start", theme::BUTTON_MENU, "Play again"}, {"B", theme::BUTTON_B, "Players"} });
	imgui.doText(Vector2(400, 468), "Keyboard:  Enter play again   Esc players", TF_SMALL_FONT | TF_ALIGN_CENTER, theme::TEXT_DIM);
}

const char* KnockoutGameState::getStateName() const
{
	return "KnockoutGameState";
}
