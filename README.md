# Lonely Cube

Lonely Cube is a voxel game written in C++. The end goal of this
project is to create a sandbox survival game, inspired mostly by
Minecraft Beta. The goal is not to copy Minecraft, but to create a
minimalist game that focuses on creativity, cool terrain generation
and the calm, lonely sort of feeling that Minecraft Beta has.

## Aims

* The primary goal of this project is to create an immersive sandbox
game that supports both singleplayer and multiplayer.

* The game should remain simple, only adding blocks that have a
unique purpose, with intuitive game mechanics and zero bloat.

* The graphics of the game are a big focus, as the game should be
beautiful to look at. This will make playing the game much more
enjoyable and immersive.

* I want the performance to remain very good which allows massive
render distances, enhancing the feeling of being in a completely open
world. The game should also perform well on low end hardware.

## Building

The game currently supports building on Linux and Windows using
CMake and GCC. Download the repo, then navigate to the top-level
folder in a terminal and run the following commands to build the
project:

### Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
cmake --build build
```
