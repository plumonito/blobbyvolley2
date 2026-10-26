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

#include "BlobbyApp.h"
#include "DuelMatch.h"
#include "GameConstants.h"
#include "IMGUI.h"
#include "InputDevice.h"
#include "InputManager.h"
#include "IUserConfigReader.h"
#include "KnockoutSetupState.h"
#include "LocalInputSource.h"
#include "replays/ReplayRecorder.h"
#include "SoundManager.h"
#include "SpeedController.h"
#include "SwitchableInputSource.h"

/* implementation */

namespace
{
	/// the knockout rules decide the winner, so the rule script must never end the match
	const int KNOCKOUT_SCORE_TO_WIN = 999999;
	/// how long "player x is out" is shown
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
	, mRecorder(new ReplayRecorder())
{
}

KnockoutGameState::~KnockoutGameState() = default;

void KnockoutGameState::init()
{
	std::shared_ptr<IUserConfigReader> config = IUserConfigReader::createUserConfigReader("config.xml");

	const KnockoutPlayer& left = mPlayers[mQueue.getActive(LEFT_PLAYER)];
	const KnockoutPlayer& right = mPlayers[mQueue.getActive(RIGHT_PLAYER)];
	setDefaultReplayName(left.getName(), right.getName());

	// set speed
	SpeedController::getMainInstance()->setGameSpeed( (float)config->getInteger("gamefps") );

	playSound(SoundManager::WHISTLE, ROUND_START_SOUND_VOLUME);

	mMatch.reset(new DuelMatch( false, config->getString("rules"), KNOCKOUT_SCORE_TO_WIN ));

	for (const auto& player : mPlayers)
		mPlayerInputs.push_back(std::make_shared<LocalInputSource>(player.controller.createInputDevice(&getInputMgr())));

	mCourtInputs[LEFT_PLAYER] = std::make_shared<SwitchableInputSource>();
	mCourtInputs[RIGHT_PLAYER] = std::make_shared<SwitchableInputSource>();

	mMatch->setPlayers(PlayerIdentity(left.getName(), left.getColor(), false, LEFT_PLAYER),
					   PlayerIdentity(right.getName(), right.getColor(), false, RIGHT_PLAYER));
	mMatch->setInputSources(mCourtInputs[LEFT_PLAYER], mCourtInputs[RIGHT_PLAYER]);
	showPlayersOnCourt();
	mWaiting.setQueue(mQueue.getWaiting());

	mRecorder->setPlayerNames(left.getName(), right.getName());
	mRecorder->setPlayerColors( left.getColor(), right.getColor() );
	mRecorder->setGameSpeed((float)config->getInteger("gamefps"));
	mRecorder->setGameRules( config->getString("rules") );
}

void KnockoutGameState::step_impl()
{
	IMGUI& imgui = getIMGUI();

	bool startPressed = gamepadStartPressed();
	// polled every frame, so that presses are detected correctly after a pause
	auto controllerPresses = mControllers.poll(getInputMgr());

	if(!mErrorMessage.empty())
	{
		displayErrorMessageBox();
	}
	else if (mSaveReplay)
	{
		if ( displaySaveReplayPrompt() )
		{
			saveReplay( *mRecorder );
		}
	}
	else if (mMatch->isPaused())
	{
		if (startPressed)
			mMatch->unpause();

		displayQueryPrompt(200,
			TextManager::LBL_CONF_QUIT,
			std::make_tuple(TextManager::LBL_YES, [&](){ switchState(new MainMenuState); }),
			std::make_tuple(TextManager::LBL_NO,  [&](){ mMatch->unpause(); }),
			std::make_tuple(TextManager::RP_SAVE, [&](){ mSaveReplay = true; imgui.resetSelection(); }));

		imgui.doCursor();
	}
	else if (mWinnerSide != NO_PLAYER)
	{
		displayWinningPlayerScreen( mWinnerSide );
		imgui.doText(GEN_ID, Vector2(500, 430), "gamepad start: play again", TF_SMALL_FONT | TF_ALIGN_CENTER);
		if (startPressed)
		{
			switchState(new KnockoutGameState(mPlayers, mLives));
		}
		if (imgui.doButton(GEN_ID, Vector2(290, 340), TextManager::LBL_OK))
		{
			switchState(new MainMenuState());
		}
		if (imgui.doButton(GEN_ID, Vector2(370, 340), TextManager::GAME_TRY_AGAIN))
		{
			switchState(new KnockoutGameState(mPlayers, mLives));
		}
		if (imgui.doButton(GEN_ID, Vector2(610, 340), "players"))
		{
			switchState(new KnockoutSetupState(mPlayers, mLives));
		}
		if (imgui.doButton(GEN_ID, Vector2(500, 390), TextManager::RP_SAVE, TF_ALIGN_CENTER))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
	}
	else if (is_exiting() || startPressed)
	{
		mMatch->pause();
	}
	else
	{
		handleJoins(controllerPresses);

		mRecorder->record(mMatch->getState());
		mMatch->step();
		updateKnockout();
		stepWaitingPlayers();

		int winner = mQueue.getWinner();
		if (winner != -1)
		{
			mWinnerSide = mQueue.getActive(LEFT_PLAYER) == winner ? LEFT_PLAYER : RIGHT_PLAYER;
			mMatch->getPlayer(mWinnerSide).setName(mPlayers[winner].getName());
			mRecorder->record(mMatch->getState());
			mRecorder->finalize( mMatch->getScore(LEFT_PLAYER), mMatch->getScore(RIGHT_PLAYER) );
		}

		if (mMessageFrames > 0)
			--mMessageFrames;

		presentGame();
	}

	presentKnockoutUI();
}

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
		{
			mMessage = mPlayers[player].getName() + " is out!";
			mMessageFrames = MESSAGE_FRAMES;
		}

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

		mPlayers.push_back(KnockoutPlayer{controller, number});
		mPlayerInputs.push_back(std::make_shared<LocalInputSource>(controller.createInputDevice(&getInputMgr())));

		mMessage = mPlayers.back().getName() + " joined";
		mMessageFrames = MESSAGE_FRAMES;
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

		if (active != mAnnouncedOnCourt[side] && mQueue.getWinner() == -1)
		{
			announceTurn(active);
			mAnnouncedOnCourt[side] = active;
		}

		PlayerIdentity& identity = mMatch->getPlayer(side);
		identity.setName(getCourtName(active));
		identity.setStaticColor(mPlayers[active].getColor());
	}
}

void KnockoutGameState::announceTurn(int player)
{
	const LocalController& controller = mPlayers[player].controller;
	if (controller.type == LocalController::GAMEPAD)
		getInputMgr().rumbleGamepad(controller.id, TURN_RUMBLE_STRENGTH, TURN_RUMBLE_MS);
}

std::string KnockoutGameState::getCourtName(int player) const
{
	std::string name = mPlayers[player].getName();
	if (mLives > 1)
		name += " (" + std::to_string(mQueue.getLives(player)) + ")";
	return name;
}

void KnockoutGameState::presentKnockoutUI()
{
	const bool running = mWinnerSide == NO_PLAYER;

	// the waiting players walk around at the back of the beach, the next one in front
	std::vector<WaitingPlayers::View> waiting;
	std::vector<RenderManager::WaitingBlob> waitingBlobs;
	if (running)
	{
		waiting = mWaiting.getViews();
		for (const auto& view : waiting)
			waitingBlobs.push_back(RenderManager::WaitingBlob{view.position, view.scale, mPlayers[view.player].getColor()});
	}

	RenderManager& renderer = getApp().getRenderManager();
	renderer.setWaitingBlobs(waitingBlobs);
	renderer.drawGame(mMatch->getState());
	renderer.setWaitingBlobs({});

	auto& imgui = getIMGUI();

	for (const auto& view : waiting)
	{
		Vector2 top = view.position - Vector2(0, BLOBBY_HEIGHT * view.scale / 2 + 14);
		imgui.doText(GEN_ID, top, std::to_string(mPlayers[view.player].number), TF_SMALL_FONT | TF_ALIGN_CENTER);
		if (view.player == mQueue.getWaiting().front())
			imgui.doText(GEN_ID, top - Vector2(0, 14), "next", TF_SMALL_FONT | TF_ALIGN_CENTER);
	}

	imgui.doText(GEN_ID, Vector2(24, 24), std::to_string(mQueue.getRemaining()) + " left");
	imgui.doText(GEN_ID, Vector2(400, 24), mMatch->getTimeString(), TF_ALIGN_CENTER);

	// players on court
	if (running)
	{
		imgui.doText(GEN_ID, Vector2(12, 550), mMatch->getPlayer(LEFT_PLAYER).getName());
		imgui.doText(GEN_ID, Vector2(788, 550), mMatch->getPlayer(RIGHT_PLAYER).getName(), TF_ALIGN_RIGHT);
	}

	if (running && KnockoutPlayer::freeNumber(mPlayers) != 0)
		imgui.doText(GEN_ID, Vector2(400, 52), "press jump to join", TF_SMALL_FONT | TF_ALIGN_CENTER);

	if (mMessageFrames > 0 && running)
		imgui.doText(GEN_ID, Vector2(400, 110), mMessage, TF_ALIGN_CENTER);
}

bool KnockoutGameState::gamepadStartPressed()
{
	bool held = false;
	for (const auto& player : mPlayers)
	{
		if (player.controller.type == LocalController::GAMEPAD)
			held = held || getInputMgr().isGamepadButtonDown(player.controller.id, SDL_CONTROLLER_BUTTON_START);
	}

	bool pressed = held && !mLastStartButton;
	mLastStartButton = held;
	return pressed;
}

const char* KnockoutGameState::getStateName() const
{
	return "KnockoutGameState";
}
