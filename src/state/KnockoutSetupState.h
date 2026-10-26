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

#include <map>
#include <vector>

#include "State.h"
#include "LocalPlayers.h"
#include "SquishAnimation.h"

/*! \class KnockoutSetupState
	\brief the menu in which up to eight local players join a knockout match
	\details Every keyboard layout and gamepad joins by pressing jump and leaves with
			down (keyboard) or B (gamepad). With left / right, a player chooses their profile.
			Y sets the number of lives, X switches drunk mode.
			The players enter the court in the order in which they joined.
*/
class KnockoutSetupState : public State
{
	public:
		explicit KnockoutSetupState(std::vector<KnockoutPlayer> players = {}, KnockoutSettings settings = KnockoutSettings());
		~KnockoutSetupState() override;

		void step_impl() override;
		const char* getStateName() const override;

	private:
		/// applies the buttons the controller pressed in this frame
		void handleController(const LocalController& controller, const LocalControllerButtons& pressed);
		KnockoutPlayer* findPlayer(const LocalController& controller);
		/// the next free profile from index start on, in the given direction; -1 if there is none
		int findFreeProfile(int start, int direction) const;
		void changeProfile(KnockoutPlayer& player, int direction);
		void changeLives();
		bool canStart() const;

		void drawPlayerCard(int slot, float x, float y);

		std::vector<KnockoutPlayer> mPlayers;
		KnockoutSettings mSettings;
		std::vector<PlayerProfile> mProfiles;

		LocalControllerWatcher mControllers;
		/// blobs on the cards squish when their player presses jump, by player number
		std::map<int, SquishAnimation> mSquish;

		/// start the match / leave the menu at the end of this frame
		bool mStartRequested = false;
		bool mBackRequested = false;
		/// leaving the game needs a second press within this many frames
		int mQuitConfirmFrames = 0;
		/// the keys may still be held from the screen that led here
		bool mLastReturn = true;
		bool mLastLivesKey = true;
		bool mLastDrunkKey = true;
};
