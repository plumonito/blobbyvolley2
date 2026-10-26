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
#include <functional>
#include <vector>

#include "PlayerInput.h"
#include "SquishAnimation.h"
#include "Vector.h"

/*! \class WaitingPlayers
	\brief the players of a knockout match who wait for their turn at the back of the beach
	\details Every waiting player controls a small blob that walks and jumps, but does not
			play the ball. Like on court, the blobs squish when they jump, walk and land. The order of the queue is shown by depth: the next player stands
			in the front row, everyone after them one row further back. When the queue moves
			on, the blobs move to their new rows smoothly.
*/
class WaitingPlayers
{
	public:
		/// a waiting blob as it is drawn
		struct View
		{
			int player;
			Vector2 position;	///< centre of the blob
			float scale;		///< size relative to a blob on court
			int frame;			///< squish animation frame
		};

		/// a player who left the court appears at this horizontal position, in front of the
		/// queue, and walks back to their row. Call before setQueue.
		void arrive(int player, float x);
		/// matches the blobs with the queue: players who are not waiting any more disappear,
		/// new players who did not arrive from the court appear in their row
		void setQueue(const std::deque<int>& queue);

		/// moves all blobs for one frame
		/// \param input returns the input of a player
		void step(const std::function<PlayerInput(int)>& input);

		/// all blobs, from the back to the front
		std::vector<View> getViews() const;
		/// the ground line of a row, 0 is the front row
		static float getRowGround(float row);
		/// the size of the blobs in a row, relative to a blob on court
		static float getRowScale(float row);

	private:
		struct Blob
		{
			int player;
			float x;
			float height;			///< above the ground of the row, while jumping
			float verticalSpeed;
			float row;				///< current row, changes smoothly towards targetRow
			float targetRow;
			SquishAnimation squish;
		};

		Blob* find(int player);

		std::vector<Blob> mBlobs;
};
