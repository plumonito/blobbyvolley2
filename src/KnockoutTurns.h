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

#include "Global.h"

/*! \class TurnTracker
	\brief decides when the turn of the player on a side is over in a knockout match
	\details A player keeps the blob while their side has the ball, so they may touch it
			several times. Their turn is over once they touched the ball and it passed the
			middle of the court to the other side. A ball that only reaches the net and
			falls back does not count, neither does anything while the ball is dead
			between two rallies.
*/
class TurnTracker
{
	public:
		/// call once per frame
		/// \param touched for each side, whether it validly touched the ball in this frame
		/// \param ballX horizontal position of the ball
		/// \param rallyRunning false while the ball is dead after a mistake, and in the frame it is reset
		/// \param turnOver receives for each side whether the turn of its player is over
		void update(const bool touched[MAX_PLAYERS], float ballX, bool rallyRunning, bool turnOver[MAX_PLAYERS]);

	private:
		/// the side the ball is completely on, NO_PLAYER while it is near the net or no rally is running
		PlayerSide mBallSide = NO_PLAYER;
		/// whether the side touched the ball since it came over
		bool mTouched[MAX_PLAYERS] = {false, false};
};
