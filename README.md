# Blobby Knockout

A party game for up to eight players, built on [Blobby Volley 2](http://blobbyvolley.de).
Everybody shares the two blobs on the beach: play the ball over the net, then the next
player takes over. Miss and you are out. The last blob standing wins.

## How to play

The game starts in the player menu.

* Every connected gamepad and the two keyboard layouts (`W A S D` and the arrow keys)
  join by pressing jump, and leave with B (keyboard: down). With left / right you pick
  your name from the players list (see below). Y (keyboard: `L`) sets the number of lives,
  X (keyboard: `X`) switches drunk mode: the whole game runs at 60 % speed, so everybody
  has more time to react. Start (keyboard: Enter) starts the match, once two players have
  joined.
* All players wait in one queue. The first two start on the left and right blob, the
  others wait at the back of the beach. While waiting you can walk around and jump.
  The order of the queue is shown by depth: the next player stands in front, everyone
  after them one row further back.
* You keep your blob while your side has the ball. Once you have touched it and it passes
  the middle of the court, your turn is over: you walk back to the end of the queue and
  the next player takes over your blob - on whichever side of the net it is. The new
  player starts in the middle of that side, with a "bloop" sound, and their gamepad
  vibrates briefly.
* When your side makes a mistake (ball on the ground, fourth touch), you lose a life.
  Without lives left you are out.
* New players can join at any time during the match, also with gamepads plugged in while
  playing: press jump and you queue up last (up to 8 players per match).
* Start (keyboard: Esc) pauses the match.

Gamepads use the SDL GameController layout: move with the d-pad or left stick, jump with
A, B or d-pad up. Any controller SDL recognises as a gamepad works (Xbox, PlayStation,
Switch Pro, 8BitDo, ...).

## Players and faces

List the players of your group in `data/players/players.txt`, one per line, optionally
with a face image that is wrapped around their blob:

```
Anna;anna.png
Bruno;bruno.jpg
Chloé
```

To turn photos into face textures, use `tools/faces/prepare_faces.py`: it finds the face,
turns it upright, crops it to the blob and cuts out face and hair. See
[data/players/README.md](data/players/README.md) for details. Without a list, players
are called Player 1 to Player 8 and their blobs have plain colours.

## Settings

`config.xml` in `~/.local/share/blobby` (created on the first start from `data/config.xml`)
holds the settings: `fullscreen`, `gamefps` (game speed, 75 is normal), `global_volume`,
`mute`, `background` and `rules` (a Lua script in `data/rules`).

## Build and run on Linux

1. Install the dependencies (Debian / Ubuntu):
```bash
sudo apt-get install g++ cmake libsdl2-dev libboost-dev libphysfs-dev
```
2. Build:
```bash
cmake -S . -B build
cmake --build build -j
```
3. Run it from the repository folder, so it finds its data:
```bash
build/src/blobby
```
4. Or install it into your home folder (`~/.local`, no root needed): the game is then
   started with `blobby` and shows up in the application menu.
```bash
cmake --install build
```
   The game data goes to `~/.local/share/blobby-knockout`; put your players there, e.g.
   `unzip players.zip -d ~/.local/share/blobby-knockout` for a zip with a `players` folder.
   For another place, configure with `-DCMAKE_INSTALL_PREFIX=/usr/local` (the data folder is
   built into the game, so set it when configuring, not when installing).

The unit tests are built with `-DBUILD_TESTS=ON` and run with `ctest --test-dir build`.

## Credits

Blobby Volley 2 is by Daniel Knobe, Jonathan Sieber, Erik Schultheis and others, see
AUTHORS. The menus use the font Fredoka (SIL Open Font License, see data/fonts) and the
stb libraries by Sean Barrett (public domain, see deps/stb). Licensed under the GNU GPL
version 2 or later, see COPYING.
