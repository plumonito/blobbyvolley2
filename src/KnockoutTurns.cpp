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
#include "KnockoutTurns.h"

/* includes */
#include "GameConstants.h"

/* implementation */

namespace
{
	/// beyond this distance from the middle, the ball cannot touch the net any more,
	/// so it has certainly passed to that side
	const float PASSED_NET_DISTANCE = NET_RADIUS + BALL_RADIUS;
}

void TurnTracker::update(const bool touched[MAX_PLAYERS], float ballX, bool rallyRunning, bool turnOver[MAX_PLAYERS])
{
	turnOver[LEFT_PLAYER] = false;
	turnOver[RIGHT_PLAYER] = false;

	if (!rallyRunning)
	{
		mBallSide = NO_PLAYER;
		mTouched[LEFT_PLAYER] = false;
		mTouched[RIGHT_PLAYER] = false;
		return;
	}

	for (PlayerSide side : {LEFT_PLAYER, RIGHT_PLAYER})
	{
		if (touched[side])
			mTouched[side] = true;
	}

	PlayerSide ballSide = mBallSide;
	if (ballX < NET_POSITION_X - PASSED_NET_DISTANCE)
		ballSide = LEFT_PLAYER;
	else if (ballX > NET_POSITION_X + PASSED_NET_DISTANCE)
		ballSide = RIGHT_PLAYER;

	if (mBallSide != NO_PLAYER && ballSide != mBallSide)
	{
		// the ball passed the net: the side it came from played it over
		turnOver[mBallSide] = mTouched[mBallSide];
		mTouched[mBallSide] = false;
		mTouched[ballSide] = false;
	}
	mBallSide = ballSide;
}
