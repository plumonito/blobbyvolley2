# Blobby Volley 2 ![build workflow](https://github.com/danielknobe/blobbyvolley2/actions/workflows/main.yaml/badge.svg)
**The head-to-head multiplayer ball game**

### Website
 http://blobbyvolley.de

### System requirements
Either Windows 2000 or later, Linux or MacOS

### Dedicated Server
The "Dedicaded Server" runs with a Gamespeed of 100%, which means 75 FPS

The Port for the Server is 1234.

### Knockout (local party mode, up to 8 players)
Choose **knockout** in the main menu. Up to eight players share the two blobs:

* Every connected gamepad and the two keyboard layouts (`W A S D` and the arrow keys)
  can join by pressing jump. Gamepad B or the keyboard's down key leaves again.
* All players wait in one queue. The first two start on the left and right blob, the
  others wait at the back of the beach as small blobs in their own colour. While waiting
  you can walk around and jump. The queue order is shown by depth: the next player stands
  in front, everyone after them one row further back.
* New players can join at any time during the match, also with gamepads plugged in
  while playing: press jump and you queue up last (up to 8 players per match).
* You keep your blob while your side has the ball. Once you have touched it and it passes
  the middle of the court, your turn is over: you walk back to the end of the queue and
  the next player takes over your blob - on whichever side of the net it is. Gamepads
  vibrate briefly when it is their player's turn.
* When your side makes a mistake (ball on the ground, fourth touch, ...), you lose a life.
  Without lives left you are out. The number of lives (1-5) is set in the menu
  (click it, or press Y on a gamepad).
* The last player standing wins.

Gamepads use the SDL GameController layout: move with the d-pad or left stick, jump with
A, B or d-pad up, Start pauses / starts the match, Back returns to the main menu.
Any controller SDL recognises as a gamepad works (Xbox, PlayStation, Switch Pro,
8BitDo, ...). The game rules (`rules` setting) still decide what counts as a mistake.

### Source Code
Clone the git repository:
```bash
git clone https://github.com/danielknobe/blobbyvolley2.git
```

### Build under Linux
Blobby Volley compiles with GCC 5 or newer, or Clang 10 or newer.
Other compilers may work but are currently untested. The minimum supported
CMake version is 3.11.

1. Install dependencies:

Debian-based Distros:
```bash
apt-get install g++ cmake libsdl2-dev libboost-dev libphysfs-dev
```
Arch-based Distros:
```bash
pacman -S gcc cmake sdl2 boost physfs
```
2. Compile:
```bash
cmake .
make
```
(An out-of-source build works as well: `cmake -S . -B build && cmake --build build -j`,
then start the game from the repository root with `build/src/blobby`.)
3. Run:
```bash
src/blobby
```

### Build under Windows 7 or newer and Visual Studio 2015 Update 3 or newer
1. Install vcpkg by following the instructions:
https://github.com/microsoft/vcpkg/blob/master/README.md

2. Install dependencies in project directory:
```powershell
.\vcpkg install
```

3. Configure, build and run:
https://github.com/microsoft/vcpkg/blob/master/docs/users/buildsystems/cmake-integration.md


### Build under MacOS
1. Install homebrew by following the instructions:
https://brew.sh

2. Install dependencies:
```bash
brew install sdl2 physfs boost
```
3. Compile:
```bash
cmake .
make
```
4. Run:
```bash
src/blobby
```

### Build for Nintendo Switch

1. [devkitPro](https://switchbrew.org/wiki/Setting_up_Development_Environment) needs to be installed and completely configured.

2. Install dependencies through devkitPro's package manager:

- SDL2
- PhysFS
- OpenGL

2. Compile:

```bash
cmake . -DCMAKE_TOOLCHAIN_FILE=NintendoSwitchToolchain.cmake -DCMAKE_INSTALL_PREFIX=blobby -DSWITCH=true -DCMAKE_BUILD_TYPE=Debug
make && make install
```

3. Copy the `blobby` folder to the `/switch/` directory on your SD card.

### Credits
See AUTHORS
