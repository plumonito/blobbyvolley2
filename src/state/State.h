/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

#include <memory>
#include <string>
#include <vector>

#include "Color.h"

class BlobbyApp;
class IMGUI;
class InputManager;
class RenderManager;

/*! \class State
	\brief Base class for all programme states.
	\details A program state describes which state a programme is in
			( e.g. the setup menu or the match ). It defines
			an abstract step function which is called each frame for the
			currently active state.
			Switching to a new state is a little cumbersome right now, as
			it requires deleting the current State (deleteCurrentState())
			and setting the new state afterwards. This approach is
			very error prone and generally not nice, so I hope we can
			replace it someday with something better ;)
*/
class State
{
public:
	virtual ~State() = default;

	// step function defines the steps actual work
	virtual void step_impl() = 0;
	// this is called just before the step function is invoked for the first time.
	virtual void init() {}
	virtual const char* getStateName() const = 0;

protected:
	State();

	BlobbyApp& getApp() const;
	IMGUI& getIMGUI() const;
	InputManager& getInputMgr() const;
	RenderManager& getRenderer() const;

	// shortcuts
	void playSound(const std::string& sound, float volume);
	/// true in the frame in which escape was pressed
	bool is_exiting() const;

	/// a controller button and what it does, shown at the bottom of the screen
	struct Hint
	{
		std::string button;		///< "A", "B", "Y", "Start", ...
		Color color;
		std::string label;
		bool enabled = true;
	};
	/// draws a row of hints, centred horizontally, with its top at y
	void doHints(float y, const std::vector<Hint>& hints);

	void switchState(State* newState);
private:

	friend class BlobbyApp;
	void setApp(BlobbyApp* app);

	BlobbyApp* m_App = nullptr;

};
