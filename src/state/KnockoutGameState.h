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
#include <vector>

#include "State.h"
#include "Global.h"
#include "KnockoutQueue.h"
#include "KnockoutTurns.h"
#include "LocalPlayers.h"
#include "SquishAnimation.h"
#include "WaitingPlayers.h"

class DuelMatch;
class InputSource;
class SwitchableInputSource;

/*! \class KnockoutGameState
	\brief local knockout match of up to eight players sharing the two blobs
	\details see KnockoutQueue for the rules. Controllers that are not playing yet can join
			at any time by pressing jump.
*/
class KnockoutGameState : public State
{
	public:
		KnockoutGameState(std::vector<KnockoutPlayer> players, KnockoutSettings settings);
		~KnockoutGameState() override;

		// implementation of the State Interface
		void step_impl() override;
		void init() override;
		const char* getStateName() const override;

	private:
		/// feeds the turns and mistakes of the last frame into the knockout rules
		void updateKnockout();
		/// lets the waiting players walk and jump
		void stepWaitingPlayers();
		/// lets controllers that pressed jump and are not part of the match yet join the queue
		void handleJoins(const LocalControllerWatcher::Presses& presses);
		bool hasJoined(const LocalController& controller) const;
		/// connects the blobs to the players on court and shows them. A player who just came
		/// on court starts in the middle of their side, with a sound.
		void showPlayersOnCourt();
		/// tells a player that it is their turn by letting their gamepad vibrate
		void announceTurn(int player);
		void showMessage(const std::string& text);
		/// sounds of the last frame
		void playSounds();

		/// draws the match and the interface on top of it
		void presentMatch();
		void presentPause();
		void presentWinner();
		/// name tag of the player on court on one side
		void doNameTag(PlayerSide side);

		std::vector<KnockoutPlayer> mPlayers;
		KnockoutSettings mSettings;
		KnockoutQueue mQueue;
		TurnTracker mTurns;
		WaitingPlayers mWaiting;

		std::unique_ptr<DuelMatch> mMatch;
		std::vector<std::shared_ptr<InputSource>> mPlayerInputs;
		/// the look of every player's blob, see RenderManager::getBlobSkin
		std::vector<int> mSkins;
		std::shared_ptr<SwitchableInputSource> mCourtInputs[MAX_PLAYERS];
		/// the players whose turn was announced last, to notice who has just come on court
		int mAnnouncedOnCourt[MAX_PLAYERS] = {-1, -1};
		int mLastTouchCount[MAX_PLAYERS] = {0, 0};

		LocalControllerWatcher mControllers;
		/// the enter key may still be held from the setup menu
		bool mLastReturn = true;

		/// e.g. "Anna is out!", shown for a moment
		std::string mMessage;
		int mMessageFrames = 0;

		bool mPaused = false;
		PlayerSide mWinnerSide = NO_PLAYER;
		/// the winner bounces on the winning screen
		SquishAnimation mWinnerSquish;
		int mWinnerFrames = 0;
};
