# Grim Dawn Riftgame Companion
Born from the idea of having "just one button to teleport me back to my personal riftgate" evolved into a full riftgate manager.  
Only works for the x64 version of the game.

## Features
* No need to search riftgates on the map
* Teleport to your (or other players) personal riftgates
* Teleport to any (unlocked) riftgate, searchable and sortable (e.g. by distance)
* Separate list for towns
* Set a favorite town and teleport to it via a single button press
* Teleport to nearest town via a single button press
* Automatically creates a personal riftgate when teleporting (as the game does)

## Additional Features
Some minor additions which are not necessary related to riftgates:
* Setting for a convenient town teleportation (to save you from walking those 10 seconds all the time)
* Restore Quest tracking status (when you untrack a quest, it will be untracked when you reload the game)
* Text notification for Shattered Realm floor completion

## Usage
Download the latest release and extract it (winmm.dll and RiftgateComapion.dll) to your x64 folder inside the Grim Dawn installation.  
The winmm.dll loader will also try to automatically load DPYes.dll if found.  
If you have any other methods of injecting dlls into your game (SpecialK or manually inject dlls with dllinjectors) you should be able to do so as well. Then you do not need to copy winmm.dll.

### Linux
Add `WINEDLLOVERRIDES="winmm=n,b" %command%` to your games steam launch option (or similar for other launchers with GOG).  
If you use `Play Grim Dawn (x64) - Compatibility Mode (SteamOS/Linux)` you might have to copy both dlls to the compat folder inside the Grim Dawn installation as well (I haven't tested this yet).

## Building
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

Copy build/winmm.dll and build/RiftgameCompanion.dll to the x64 directory inside Grim Dawns install directory.
