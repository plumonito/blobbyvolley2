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
#include "Crowd.h"

/* includes */
#include <algorithm>
#include <cmath>

#include "GameConstants.h"

/* implementation */

namespace
{
	const float LEFT_BORDER = 30;
	const float RIGHT_BORDER = 770;
	/// how far a spectator shuffles away from their place
	const float SHUFFLE_RANGE = 18;
	const float SHUFFLE_SPEED = 0.6;
	/// chance per frame to start shuffling somewhere else
	const float SHUFFLE_CHANCE = 1 / 150.f;
	/// chance per frame to start hopping by themselves
	const float HOP_CHANCE = 1 / 200.f;

	const float MIN_JUMP_SPEED = 3.5;
	const float MAX_JUMP_SPEED = 5;
	const float CROWD_GRAVITATION = 0.35;
	/// frames between two hops of a series
	const int MAX_HOP_PAUSE = 6;
	/// a cheer does not start for everybody at the same moment
	const int MAX_CHEER_DELAY = 20;
}

Crowd::Crowd(unsigned seed) : mRandom(seed)
{
}

void Crowd::setMembers(const std::vector<int>& members)
{
	mSpectators.erase(std::remove_if(mSpectators.begin(), mSpectators.end(), [&](const Spectator& spectator) {
			return std::find(members.begin(), members.end(), spectator.member) == members.end();
		}), mSpectators.end());

	std::vector<int> arriving;
	for (int member : members)
	{
		bool present = std::any_of(mSpectators.begin(), mSpectators.end(),
				[&](const Spectator& spectator) { return spectator.member == member; });
		if (!present)
			arriving.push_back(member);
	}
	if (arriving.empty())
		return;

	// spread a group that arrives together evenly over the beach, in random order, and
	// someone who arrives alone anywhere
	std::vector<float> places;
	float width = (RIGHT_BORDER - LEFT_BORDER) / arriving.size();
	for (unsigned i = 0; i < arriving.size(); ++i)
		places.push_back(LEFT_BORDER + width * (i + random(0.2f, 0.8f)));
	std::shuffle(places.begin(), places.end(), mRandom);

	for (unsigned i = 0; i < arriving.size(); ++i)
	{
		Spectator spectator;
		spectator.member = arriving[i];
		spectator.home = spectator.x = spectator.target = places[i];
		spectator.depth = random(0, 1);
		spectator.delay = (int)random(0, 60);
		mSpectators.push_back(spectator);
	}
}

void Crowd::cheer()
{
	for (auto& spectator : mSpectators)
	{
		spectator.hops = std::max(spectator.hops, (int)random(2, 5));
		spectator.delay = (int)random(0, MAX_CHEER_DELAY);
	}
}

void Crowd::step()
{
	std::uniform_real_distribution<float> chance(0, 1);
	for (auto& spectator : mSpectators)
	{
		bool onGround = spectator.height <= 0 && spectator.verticalSpeed <= 0;

		// shuffle around their place
		if (chance(mRandom) < SHUFFLE_CHANCE)
			spectator.target = std::max(LEFT_BORDER, std::min(spectator.home + random(-SHUFFLE_RANGE, SHUFFLE_RANGE), RIGHT_BORDER));
		if (std::abs(spectator.target - spectator.x) <= SHUFFLE_SPEED)
			spectator.x = spectator.target;
		else
			spectator.x += spectator.target > spectator.x ? SHUFFLE_SPEED : -SHUFFLE_SPEED;

		// hop, by themselves now and then, or a few times in a row when cheering
		if (onGround && spectator.hops == 0 && chance(mRandom) < HOP_CHANCE)
			spectator.hops = (int)random(1, 3);
		if (spectator.delay > 0)
			--spectator.delay;
		else if (onGround && spectator.hops > 0)
			jump(spectator);

		if (!onGround)
		{
			spectator.height += spectator.verticalSpeed;
			spectator.verticalSpeed -= CROWD_GRAVITATION;
			if (spectator.height <= 0)
			{
				// squish when landing, like the players do
				spectator.height = 0;
				spectator.verticalSpeed = 0;
				spectator.squish.start();
				spectator.delay = (int)random(0, MAX_HOP_PAUSE);
			}
		}
		spectator.squish.step();
	}
}

std::vector<Crowd::View> Crowd::getViews() const
{
	std::vector<Spectator> spectators = mSpectators;
	std::stable_sort(spectators.begin(), spectators.end(),
			[](const Spectator& a, const Spectator& b) { return a.depth > b.depth; });

	std::vector<View> views;
	for (const auto& spectator : spectators)
	{
		float scale = FRONT_SCALE + (BACK_SCALE - FRONT_SCALE) * spectator.depth;
		float ground = FRONT_GROUND + (BACK_GROUND - FRONT_GROUND) * spectator.depth;
		Vector2 centre(spectator.x, ground - spectator.height - BLOBBY_HEIGHT * scale / 2);
		views.push_back(View{spectator.member, centre, scale, spectator.squish.frame()});
	}
	return views;
}

float Crowd::random(float min, float max)
{
	return std::uniform_real_distribution<float>(min, max)(mRandom);
}

void Crowd::jump(Spectator& spectator)
{
	spectator.verticalSpeed = random(MIN_JUMP_SPEED, MAX_JUMP_SPEED);
	spectator.height = spectator.verticalSpeed;
	spectator.verticalSpeed -= CROWD_GRAVITATION;
	--spectator.hops;
	// squish when jumping off
	spectator.squish.start();
}
