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

#include <deque>
#include <vector>

#include "Global.h"

/*! \class KnockoutQueue
	\brief rules of a knockout match between several players who share the two blobs
	\details All players wait in one queue. Two of them are on court, one on each side.
			When the turn of the player on a side is over (see TurnTracker), they go to the back
			of the queue and the first waiting player takes over that blob, no matter which side
			it is on.
			When a side makes a mistake, the player on court there loses a life and leaves the
			court as well; without lives left, they are knocked out instead of queueing up again.
			Once nobody is waiting, the last two players stay on court until one of them is out.
			New players can join at any time before the match is decided: they queue up last.
			The last player standing wins.
			Players are identified by their index, in the order in which they joined.
*/
class KnockoutQueue
{
	public:
		/// players 0 and 1 start on the left and right side, the others wait in ascending order
		KnockoutQueue(int playerCount, int lives);

		/// applies the events of one frame of the match
		/// \param turnOver for each side, whether the turn of its player is over
		/// \param error for each side, whether it made a mistake
		/// \return true if the players on court or their lives changed
		bool update(const bool turnOver[MAX_PLAYERS], const bool error[MAX_PLAYERS]);

		/// a new player joins the match and waits at the end of the queue
		/// \return the index of the new player, or -1 if the match is already decided
		int addPlayer(int lives);
		/// number of players who ever took part, including those knocked out
		int getPlayerCount() const { return mLives.size(); }

		/// the player who controls the blob on that side. When the last opponent is knocked
		/// out, the side keeps that player, as the match is over anyway.
		int getActive(PlayerSide side) const { return mActive[side]; }
		/// the players waiting for their turn, the first one comes in next
		const std::deque<int>& getWaiting() const { return mWaiting; }

		int getLives(int player) const { return mLives[player]; }
		bool isKnockedOut(int player) const { return mLives[player] <= 0; }
		/// number of players who are not knocked out
		int getRemaining() const;
		/// the winner once a single player is left, -1 while the match is running
		int getWinner() const;
		/// players knocked out by the last update
		const std::vector<int>& getKnockedOut() const { return mKnockedOut; }

	private:
		/// the player on that side leaves the court, if someone is waiting to take over
		bool leaveCourt(PlayerSide side);

		int mActive[MAX_PLAYERS];
		std::deque<int> mWaiting;
		std::vector<int> mLives;
		std::vector<int> mKnockedOut;
};
