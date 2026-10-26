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
#include "KnockoutQueue.h"

/* includes */
#include <cassert>

/* implementation */

KnockoutQueue::KnockoutQueue(int playerCount, int lives) : mLives(playerCount, lives)
{
	assert(playerCount >= 2);
	assert(lives >= 1);

	mActive[LEFT_PLAYER] = 0;
	mActive[RIGHT_PLAYER] = 1;
	for (int player = 2; player < playerCount; ++player)
		mWaiting.push_back(player);
}

bool KnockoutQueue::update(const bool touched[MAX_PLAYERS], const bool error[MAX_PLAYERS])
{
	mKnockedOut.clear();

	// nothing changes once the match is decided
	if (getWinner() != -1)
		return false;

	bool changed = false;
	for (PlayerSide side : {LEFT_PLAYER, RIGHT_PLAYER})
	{
		if (error[side])
		{
			// a faulty touch is handled here as well, so the player does not leave twice
			int player = mActive[side];
			--mLives[player];
			if (isKnockedOut(player))
				mKnockedOut.push_back(player);
			leaveCourt(side);
			changed = true;
		}
		else if (touched[side])
		{
			changed = leaveCourt(side) || changed;
		}
	}
	return changed;
}

int KnockoutQueue::addPlayer(int lives)
{
	assert(lives >= 1);
	if (getWinner() != -1)
		return -1;

	int player = mLives.size();
	mLives.push_back(lives);
	mWaiting.push_back(player);
	return player;
}

bool KnockoutQueue::leaveCourt(PlayerSide side)
{
	if (mWaiting.empty())
		return false;

	int player = mActive[side];
	mActive[side] = mWaiting.front();
	mWaiting.pop_front();

	if (!isKnockedOut(player))
		mWaiting.push_back(player);
	return true;
}

int KnockoutQueue::getRemaining() const
{
	int remaining = 0;
	for (int lives : mLives)
	{
		if (lives > 0)
			++remaining;
	}
	return remaining;
}

int KnockoutQueue::getWinner() const
{
	if (getRemaining() != 1)
		return -1;

	for (unsigned player = 0; player < mLives.size(); ++player)
	{
		if (!isKnockedOut(player))
			return player;
	}
	return -1;
}
