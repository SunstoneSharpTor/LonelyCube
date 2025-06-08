# Lonely Cube

Lonely Cube is a 3D voxel game, inspired mostly by Minecraft Beta. The
concept for this game is basically "Minecraft but how I imagined it",
which for me means:

* Minimalistic - Like the early versions of Minecraft, Lonely Cube
doesn't have many types of blocks. Instead it tries to foster
creativity by presenting a simple block palette that leaves the
creative power with the player rather than a texture artist.

* Cool Terrain Generation - The world should provide inspiration for
the player to build in it. This means beautiful natural blocky
landscapes, containing fancy trees and natural structures.

* Visuals - Lonely Cube has a large render distance and uses HDR
rendering to provide modern graphics and immersive gameplay.

* Performance - Lonely Cube is written in C++ and uses Vulkan 1.3 for
great performance even on old systems.

* Multiplayer - Like Minecraft, Lonely Cube supports both singleplayer
and multiplayer worlds.

The game is named Lonely Cube because it strives to recreate the
lonely feeling often felt when playing old Minecraft. No pregenerated
structures, no villagers, just you and your friends in an infinite
sandbox with no predefined goals or limits.

## Building

The game currently supports building on Linux and Windows using CMake.
Please ensure Git is installed as the CMake setup uses FetchContent to
download the git repositories of dependencies.

### Linux

To compile the project on Linux you need to have the X11, Wayland and
xkbcommon development packages installed (in order to build GLFW). On
some systems a few other packages are also required. For full details
see the [GLFW Compilation Guide](https://www.glfw.org/docs/latest/compile.html#compile_deps_wayland).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Windows

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
cmake --build build
```

#### Notes
The CMake setup supports the 4 default build types:
* `Release`: Full compiler optimisations and no debug information.
* `Debug`: Very few compiler optimisations and full debug information.
* `RelWithDebInfo`: Many compiler optimisations and enough debug
information to be able to get a backtrace.
* `MinSizeRel`: Same as release but optimising for binary size rather
than speed.
