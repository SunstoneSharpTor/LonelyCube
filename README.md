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
Download the source code and ensure you have installed the
[build prerequisites](#build-prerequisites) then run the following
commands:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
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

<a id="build-prerequisites"></a>
### Build Prerequisites

There is some software that you will need to have installed in order
to build the game. See below to install it depending on your operating
system:

#### Linux

##### Debian / Ubuntu
```sh
sudo apt install cmake git build-essential libvulkan-dev libwayland-dev libxkbcommon-dev xorg-dev
```

##### RHEL / Fedora
```sh
sudo dnf -y install cmake git gcc make glibc-gconv-extra vulkan-loader-devel wayland-devel libxkbcommon-devel libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel
```

##### Arch
```sh
sudo pacman -S cmake git base-devel vulkan-devel wayland libxkbcommon
```

#### Windows

To build the project on Windows, you will need to install the
[Vulkan SKD](https://www.lunarg.com/vulkan-sdk/) in addition to a C++
compiler, CMake, a build system and git.
