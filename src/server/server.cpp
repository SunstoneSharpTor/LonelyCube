/*
  Lonely Cube, a voxel game
  Copyright (C) 2024-2025 Bertie Cartwright

  Lonely Cube is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Lonely Cube is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "core/pch.h"

#include <enet/enet.h>

#include "core/chunk.h"
#include "core/log.h"
#include "core/packet.h"
#include "core/serverWorld.h"
#include "server/serverNetworking.h"
#include "core/resourceMonitor.h"

using namespace lonelycube;

using namespace lonelycube::server;

static void receiveCommands(bool* running) {
    std::string command;
    while (*running) {
        std::getline(std::cin, command);
        if (command == "quit") {
            *running = false;
        }
    }
}

static void chunkLoaderThread(ServerWorld<false>* mainWorld, bool* running, int8_t threadNum) {
    while (*running) {
        mainWorld->waitIfRequired(threadNum);
        IVec3 chunkPosition;
        if (mainWorld->loadNextChunk(&chunkPosition)) {

        }
    }
}

int main (int argc, char** argv) {
    ENetEvent event;
    ENetAddress address;
    ServerNetworking networking;

    if (!networking.initServer(address)) {
        LOG("Failed to initialise server");
        return 0;
    }

    uint32_t worldSeed = std::time(0);
    ServerWorld<false> mainWorld(worldSeed, networking.getMutex());
    LOG("World Seed: " + std::to_string(worldSeed));

    uint8_t numChunkLoaderThreads = mainWorld.getNumChunkLoaderThreads();
    std::vector<bool> chunkLoaderThreadsRunning(numChunkLoaderThreads);
    std::fill(chunkLoaderThreadsRunning.begin(), chunkLoaderThreadsRunning.end(), true);

    bool running = true;

    std::vector<std::thread> chunkLoaderThreads;
    for (int8_t threadNum = 0; threadNum < numChunkLoaderThreads; threadNum++)
        chunkLoaderThreads.emplace_back(chunkLoaderThread, &mainWorld, &running, threadNum);

    // Gameloop
    std::thread(receiveCommands, &running).detach();
    auto nextTick = std::chrono::steady_clock::now() + std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
    while(running) {
        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime >= nextTick) {
            mainWorld.tick();
            nextTick += std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
        }

        networking.receiveEvents(mainWorld);
    }

    chunkLoaderThreadsRunning[0] = false;

    for (int8_t threadNum = 0; threadNum < numChunkLoaderThreads; threadNum++) {
        chunkLoaderThreads[threadNum].join();
        chunkLoaderThreadsRunning[threadNum] = false;
    }

    enet_host_destroy(networking.getHost());

    enet_deinitialize();

    return 0;
}
