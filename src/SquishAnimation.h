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

/*! \struct SquishAnimation
	\brief the squish animation of a blob, as on court: it squashes down to frame 4 and back
*/
struct SquishAnimation
{
	float state = 0;
	float speed = 0;

	/// starts squishing, unless the blob is already squishing
	void start()
	{
		if (speed == 0)
			speed = SPEED;
	}

	/// advances the animation by one frame
	void step()
	{
		if (state >= 4.5f)
			speed = -SPEED;
		state += speed;
		if (state >= 5)
			state = 4.99f;
		if (state < 0)
		{
			state = 0;
			speed = 0;
		}
	}

	int frame() const { return int(state) % 5; }

	static constexpr float SPEED = 0.5f;
};
