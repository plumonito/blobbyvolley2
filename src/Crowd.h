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

#include <random>
#include <vector>

#include "SquishAnimation.h"
#include "Vector.h"

/*! \class Crowd
	\brief the spectators at the far end of the beach: everybody from the players list who
		is not playing
	\details The spectators stand behind the waiting players, smaller than them, and shuffle
			around, hop and squish by themselves. When a rally ends they all cheer for a
			moment. Nobody controls them.
*/
class Crowd
{
	public:
		/// a spectator as it is drawn
		struct View
		{
			int member;
			Vector2 position;	///< centre of the blob
			float scale;		///< size relative to a blob on court
			int frame;			///< squish animation frame
		};

		explicit Crowd(unsigned seed = std::random_device()());

		/// who is watching, e.g. indices into the players list. New members get a place in the
		/// crowd, members who are not listed any more leave it.
		void setMembers(const std::vector<int>& members);
		/// everybody jumps up and down for a moment
		void cheer();
		/// moves the spectators for one frame
		void step();

		/// all spectators, from the back to the front
		std::vector<View> getViews() const;

		/// the ground line of the front and back of the crowd, and the size of the
		/// spectators there: all smaller than the waiting players, so they look further away
		static constexpr float FRONT_GROUND = 314;
		static constexpr float BACK_GROUND = 296;
		static constexpr float FRONT_SCALE = 0.32f;
		static constexpr float BACK_SCALE = 0.25f;

	private:
		struct Spectator
		{
			int member;
			float home;				///< where they stand, they shuffle around it
			float x;
			float target;			///< where they shuffle to
			float depth;			///< 0 front of the crowd, 1 back
			float height = 0;		///< above the ground, while jumping
			float verticalSpeed = 0;
			int hops = 0;			///< jumps still to come, one after another
			int delay = 0;			///< frames until the next jump
			SquishAnimation squish;
		};

		float random(float min, float max);
		void jump(Spectator& spectator);

		std::vector<Spectator> mSpectators;
		std::mt19937 mRandom;
};
