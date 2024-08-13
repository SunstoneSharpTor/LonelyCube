/*
  Lonely Cube, a voxel game
  Copyright (C) 2024 Bertie Cartwright

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "core/pch.h"
#include <cstring>

#include <enet/enet.h>

#include "core/chunk.h"
#include "core/packet.h"
#include "core/serverWorld.h"
#include "server/serverNetworking.h"

using namespace server;

void receiveCommands(bool* running) {
    std::string command;
    while (*running) {
        std::getline(std::cin, command);
        if (command == "quit") {
            *running = false;
        }
    }
}

void chunkLoaderThread(ServerWorld<false>* mainWorld, bool* running, char threadNum) {
    while (*running) {
        mainWorld->waitIfRequired(threadNum);
        Position chunkPosition;
        if (mainWorld->loadChunk(&chunkPosition)) {
            
        }
    }
}

int main (int argc, char** argv) {
    ENetEvent event;
    ENetAddress address;
    ServerNetworking networking;

    if (!networking.initServer(address)) {
        std::cout << "Failed to initialise server" << std::endl;
        return 0;
    }

    unsigned int worldSeed = std::time(0);
    ServerWorld<false> mainWorld(worldSeed);
    std::cout << "World Seed: " << worldSeed << std::endl;

    unsigned char numChunkLoaderThreads = mainWorld.getNumChunkLoaderThreads();
    bool* chunkLoaderThreadsRunning = new bool[numChunkLoaderThreads];
    std::fill(chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + numChunkLoaderThreads, true);

    bool running = true;

    std::thread* chunkLoaderThreads = new std::thread[numChunkLoaderThreads];
    for (char threadNum = 0; threadNum < numChunkLoaderThreads; threadNum++) {
        chunkLoaderThreads[threadNum] = std::thread(chunkLoaderThread, &mainWorld, &running, threadNum);
    }

    // Gameloop
    std::thread(receiveCommands, &running).detach();
    auto nextTick = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    while(running) {
        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime >= nextTick) {
            mainWorld.tick();
            nextTick += std::chrono::milliseconds(100);
        }
        networking.receiveEvents(mainWorld);
    }

    chunkLoaderThreadsRunning[0] = false;

    for (char threadNum = 0; threadNum < numChunkLoaderThreads; threadNum++) {
        chunkLoaderThreads[threadNum].join();
        chunkLoaderThreadsRunning[threadNum] = false;
    }

    enet_host_destroy(networking.getHost());

    enet_deinitialize();

    return 0;
}