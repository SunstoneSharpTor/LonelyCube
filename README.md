# Lonely Cube

Lonely Cube is a Minecraft-like voxel game written in C++. The end
goal of this project is to create a sandbox survival game, inspired
mostly by Minecraft Beta. The goal is not to copy Minecraft, but to
create a minimalist game that focuses on creativity, cool terrain
generation and the lonely sort of feeling that Minecraft Beta has.

## Aims

* The primary goal of this project is to create an immersive sandbox
game that supports both singleplayer and multiplayer.

* The game should remain simple, only adding blocks that have a
unique purpose, with intuitive game mechanics and zero bloat.

* The graphics of the game are a big focus, as the game should be
beautiful to look at. This will make playing the game much more
enjoyable and immersive.

* I want the performance to remain much better than Minecraft's,
which allows massive render distances, enhancing the feeling of being
in a completely open world where you can do whatever you want.

## Building

The game currently supports building on Linux and Windows using
CMake and GCC. You will need to have SDL2 development packages
installed before building. You can find how to do that
[here](https://wiki.libsdl.org/SDL2/Installation). On Windows, ensure
that the CMAKE_PREFIX_PATH environment variable points to a folder
containing a version of the SDL2-devel-2.x.x-mingw folder.

Download the repo, then navigate to the top-level folder in a command
line/terminal and run the following commands to build the project:

```sh
cmake .
make
```