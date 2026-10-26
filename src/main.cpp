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

/* includes */
#include <ctime>
#include <cstring>
#include <sstream>
#include <atomic>
#include <thread>
#include <iostream>

#include <SDL.h>

#include "Global.h"

#include "RenderManager.h"
#include "SoundManager.h"
#include "InputManager.h"
#include "UserConfig.h"
#include "IMGUI.h"
#include "SpeedController.h"
#include "FileSystem.h"
#include "state/KnockoutSetupState.h"
#include "BlobbyApp.h"


/* implementation */

void deinit()
{
	SDL_Quit();

}

void setupPHYSFS()
{
	FileSystem& fs = FileSystem::getSingleton();
	
	// Game should be playable out of the source package on all
	// relevant platforms.

	// List all the subdirectories / zip archives that we want to read from
	std::vector<std::string> subdirs = {"gfx", "sounds", "backgrounds", "rules", "fonts", "players"};

	// set write dir
	std::string writeDir = fs.getPrefDir();
	fs.setWriteDir(writeDir);
	for(const auto& sub_dir : subdirs) {
		fs.probeDir(sub_dir);
	}
	fs.probeDir("replays");

	// set read dir (files in datafolder next to working dir)
	{
		std::string dataDir = "data";
		fs.addToSearchPath(dataDir);
		for(const auto& archive : subdirs) {
			fs.addToSearchPath(fs.join(dataDir, archive + ".zip"));
		}
	}

	// set read dir (files next to application)
	{
		std::string dataDir = fs.getBaseDir();
		fs.addToSearchPath(dataDir);
		for(const auto& archive : subdirs) {
			fs.addToSearchPath(fs.join(dataDir, archive + ".zip"));
		}
	}

	// set read dir (unix-like when installed)
	#ifdef BLOBBY_DATA_DIR
		fs.addToSearchPath(BLOBBY_DATA_DIR);
		for(const auto& archive : subdirs) {
			fs.addToSearchPath(fs.join(BLOBBY_DATA_DIR, archive + ".zip"));
		}
	#endif
}

int main(int argc, char* argv[])
{
	DEBUG_STATUS("started main");

	FileSystem filesys(argv[0]);
	setupPHYSFS();

	DEBUG_STATUS("physfs initialised");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);


	DEBUG_STATUS("SDL initialised");

	atexit(SDL_Quit);
	srand(SDL_GetTicks());


	try
	{
		UserConfig gameConfig;
		gameConfig.loadFile("config.xml");

		SpeedController scontroller(gameConfig.getFloat("gamefps"));
		SpeedController::setMainInstance(&scontroller);
		scontroller.setDrawFPS(gameConfig.getBool("showfps"));

		int running = 1;

		DEBUG_STATUS("starting mainloop");

		BlobbyApp app( std::unique_ptr<State>(new KnockoutSetupState()), gameConfig );

		while (running)
		{
			app.getInputManager().updateInput();
			running = app.getInputManager().running();

			app.getIMGUI().begin();
			app.step();
			//draw FPS:
			static int lastfps = 0;
			if (scontroller.getDrawFPS())
			{
				// We need to ensure that the title bar is only set
				// when the framerate changed, because setting the
				// title can be quite resource intensive on some
				// windows manager, like for example metacity.
				int newfps = scontroller.getFPS();
				if (newfps != lastfps)
				{
					std::stringstream tmp;
					tmp << AppTitle << "  FPS: " << newfps;
					app.getRenderManager().setTitle(tmp.str());
				}
				lastfps = newfps;
			}
			// Dirty workarround for hiding fps in title
			if (!scontroller.getDrawFPS() && (lastfps != -1))
			{
				std::stringstream tmp;
				tmp << AppTitle;
				app.getRenderManager().setTitle(tmp.str());

				lastfps = -1;
			}

			if (!scontroller.doFramedrop())
			{
				app.getIMGUI().end(app.getRenderManager());
				app.getRenderManager().refresh();
			}
			scontroller.update();
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", (std::string("An error occurred, blobby will close: ") + e.what()).c_str(), nullptr);
		SDL_Quit();
		exit (EXIT_FAILURE);
	}
	deinit();
	exit(EXIT_SUCCESS);
}
