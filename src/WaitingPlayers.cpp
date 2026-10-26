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
#include "WaitingPlayers.h"

/* includes */
#include <algorithm>
#include <cmath>

#include "GameConstants.h"

/* implementation */

namespace
{
	const float FRONT_ROW_GROUND = 352;
	const float ROW_DISTANCE = 9;
	const float FRONT_ROW_SCALE = 0.55;
	const float ROW_SCALE_STEP = 0.03;
	/// rows per frame when a blob moves to another row
	const float ROW_CHANGE_SPEED = 0.08;
	/// row of players coming from the court, in front of the queue
	const float ARRIVAL_ROW = -1.5;

	const float WALK_SPEED = 3;
	const float JUMP_SPEED = 9;
	const float WAITING_GRAVITATION = 0.6;
	const float LEFT_BORDER = 40;
	const float RIGHT_BORDER = 760;
}

void WaitingPlayers::arrive(int player, float x)
{
	if (find(player) != nullptr)
		return;
	mBlobs.push_back(Blob{player, std::max(LEFT_BORDER, std::min(x, RIGHT_BORDER)), 0, 0, ARRIVAL_ROW, ARRIVAL_ROW});
}

void WaitingPlayers::setQueue(const std::deque<int>& queue)
{
	mBlobs.erase(std::remove_if(mBlobs.begin(), mBlobs.end(), [&](const Blob& blob) {
			return std::find(queue.begin(), queue.end(), blob.player) == queue.end();
		}), mBlobs.end());

	for (unsigned i = 0; i < queue.size(); ++i)
	{
		Blob* blob = find(queue[i]);
		if (blob == nullptr)
		{
			// spread new players over the beach, so they do not stand on top of each other
			float x = LEFT_BORDER + 80 + std::fmod(i * 150.f, RIGHT_BORDER - LEFT_BORDER - 160);
			mBlobs.push_back(Blob{queue[i], x, 0, 0, (float)i, (float)i});
		}
		else
		{
			blob->targetRow = i;
		}
	}
}

void WaitingPlayers::step(const std::function<PlayerInput(int)>& input)
{
	for (auto& blob : mBlobs)
	{
		PlayerInput in = input(blob.player);

		if (in.left)
			blob.x -= WALK_SPEED;
		if (in.right)
			blob.x += WALK_SPEED;
		blob.x = std::max(LEFT_BORDER, std::min(blob.x, RIGHT_BORDER));

		bool onGround = blob.height <= 0;
		if (onGround && in.up)
			blob.verticalSpeed = JUMP_SPEED;

		if (!onGround || blob.verticalSpeed > 0)
		{
			blob.height += blob.verticalSpeed;
			blob.verticalSpeed -= WAITING_GRAVITATION;
			if (blob.height <= 0)
			{
				blob.height = 0;
				blob.verticalSpeed = 0;
			}
		}

		if (std::abs(blob.targetRow - blob.row) <= ROW_CHANGE_SPEED)
			blob.row = blob.targetRow;
		else
			blob.row += blob.targetRow > blob.row ? ROW_CHANGE_SPEED : -ROW_CHANGE_SPEED;
	}
}

std::vector<WaitingPlayers::View> WaitingPlayers::getViews() const
{
	std::vector<Blob> blobs = mBlobs;
	std::stable_sort(blobs.begin(), blobs.end(), [](const Blob& a, const Blob& b) { return a.row > b.row; });

	std::vector<View> views;
	for (const auto& blob : blobs)
	{
		float scale = getRowScale(blob.row);
		Vector2 centre(blob.x, getRowGround(blob.row) - blob.height - BLOBBY_HEIGHT * scale / 2);
		views.push_back(View{blob.player, centre, scale});
	}
	return views;
}

float WaitingPlayers::getRowGround(float row)
{
	return FRONT_ROW_GROUND - row * ROW_DISTANCE;
}

float WaitingPlayers::getRowScale(float row)
{
	return FRONT_ROW_SCALE - row * ROW_SCALE_STEP;
}

WaitingPlayers::Blob* WaitingPlayers::find(int player)
{
	for (auto& blob : mBlobs)
	{
		if (blob.player == player)
			return &blob;
	}
	return nullptr;
}
