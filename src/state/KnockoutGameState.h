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

#include "GameState.h"
#include "KnockoutQueue.h"
#include "LocalPlayers.h"

class InputSource;
class ReplayRecorder;
class SwitchableInputSource;

/*! \class KnockoutGameState
	\brief local knockout match of up to eight players sharing the two blobs
	\details see KnockoutQueue for the rules. Controllers that are not playing yet can join
			at any time by pressing jump.
*/
class KnockoutGameState : public GameState
{
	public:
		KnockoutGameState(std::vector<KnockoutPlayer> players, int lives);
		~KnockoutGameState() override;

		// implementation of the State Interface
		void step_impl() override;
		void init() override;
		const char* getStateName() const override;

	private:
		/// feeds the touches and mistakes of the last frame into the knockout rules
		void updateKnockout();
		/// lets controllers that pressed jump and are not part of the match yet join the queue
		void handleJoins(const LocalControllerWatcher::Presses& presses);
		bool hasJoined(const LocalController& controller) const;
		/// connects the blobs to the players on court and shows their names and colours
		void showPlayersOnCourt();
		/// tells a player that it is their turn by letting their gamepad vibrate
		void announceTurn(int player);
		/// name shown below the blob, with the remaining lives if there is more than one
		std::string getCourtName(int player) const;
		/// draws the game ui: time, players on court, queue and messages
		void presentKnockoutUI();
		/// true in the frame in which any gamepad of the players pressed start
		bool gamepadStartPressed();

		std::vector<KnockoutPlayer> mPlayers;
		int mLives;
		KnockoutQueue mQueue;

		std::vector<std::shared_ptr<InputSource>> mPlayerInputs;
		std::shared_ptr<SwitchableInputSource> mCourtInputs[MAX_PLAYERS];
		/// the players whose turn was announced last, to notice who has just come on court
		int mAnnouncedOnCourt[MAX_PLAYERS] = {-1, -1};
		int mLastTouchCount[MAX_PLAYERS] = {0, 0};

		LocalControllerWatcher mControllers;

		/// e.g. "player 3 is out", shown for a moment
		std::string mMessage;
		int mMessageFrames = 0;

		/// the start button may still be held from the setup menu
		bool mLastStartButton = true;

		PlayerSide mWinnerSide = NO_PLAYER;

		std::unique_ptr<ReplayRecorder> mRecorder;
};
