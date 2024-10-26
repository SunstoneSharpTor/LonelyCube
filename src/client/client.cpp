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

#include <enet/enet.h>

#include "core/pch.h"
#include <time.h>

#include "client/clientNetworking.h"
#include "client/renderThread.h"
#include "client/clientWorld.h"
#include "client/clientPlayer.h"
#include "core/config.h"
#include "core/packet.h"

using namespace client;

void chunkLoaderThreadSingleplayer(ClientWorld& mainWorld, bool* running, int8_t threadNum, int*
    numThreadsBeingUsed) {
    while (*running) {
        while (threadNum >= *numThreadsBeingUsed && *running) {
            mainWorld.setThreadWaiting(threadNum, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            mainWorld.setThreadWaiting(threadNum, false);
        }
        mainWorld.loadChunksAroundPlayerSingleplayer(threadNum);
    }
}

void chunkLoaderThreadMultiplayer(ClientWorld& mainWorld, ClientNetworking& networking, bool*
    running, int8_t threadNum, int* numThreadsBeingUsed) {
    while (*running) {
        while (threadNum >= *numThreadsBeingUsed && *running) {
            mainWorld.setThreadWaiting(threadNum, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            mainWorld.setThreadWaiting(threadNum, false);
        }
        mainWorld.loadChunksAroundPlayerMultiplayer(threadNum);
        networking.receiveEvents(mainWorld);
    }
}

void manageThreads(float* chunkLoadTimes, int numChunkLoaderThreads, int* numThreadsBeingUsed) {
    if (*numThreadsBeingUsed > 1) {
        chunkLoadTimes[*numThreadsBeingUsed - 2] *= 0.95f;
    }
    if (*numThreadsBeingUsed < numChunkLoaderThreads) {
        chunkLoadTimes[*numThreadsBeingUsed] *= 0.9f;
    }
    if (*numThreadsBeingUsed > 1 &&
        chunkLoadTimes[*numThreadsBeingUsed - 2] < chunkLoadTimes[*numThreadsBeingUsed - 1]) {
        (*numThreadsBeingUsed)--;
        std::cout << *numThreadsBeingUsed << " threads being used\n";
    }
    else if (*numThreadsBeingUsed < numChunkLoaderThreads && chunkLoadTimes[*numThreadsBeingUsed] <
        chunkLoadTimes[*numThreadsBeingUsed - 1] && chunkLoadTimes[*numThreadsBeingUsed] > chunkLoadTimes[*numThreadsBeingUsed - 2]) {
        (*numThreadsBeingUsed)++;
        std::cout << *numThreadsBeingUsed << " threads being used\n";
    }
}

int main(int argc, char* argv[]) {
    Config settings("res/settings.txt");

    bool multiplayer = settings.getMultiplayer();
    
    ClientNetworking networking;

    if (multiplayer) {
        if (!networking.establishConnection(settings.getServerIP(), settings.getRenderDistance())) {
            multiplayer = false;
        }
    }

    uint32_t worldSeed = std::time(0);
    int playerSpawnPoint[3] = { 0, 200, 0 };
    ClientWorld mainWorld(settings.getRenderDistance(), worldSeed, !multiplayer, playerSpawnPoint);
    std::cout << "World Seed: " << worldSeed << std::endl;
    ClientPlayer mainPlayer(playerSpawnPoint, &mainWorld, mainWorld.getResourcePack());

    bool running = true;
    int frameTime;

    bool* chunkLoaderThreadsRunning = new bool[mainWorld.getNumChunkLoaderThreads()];
    std::fill(chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + mainWorld.getNumChunkLoaderThreads(), true);

    RenderThread renderThread(&mainWorld, chunkLoaderThreadsRunning, &mainPlayer, networking, &frameTime);

    std::thread renderWorker(&RenderThread::go, renderThread, &running);

    std::unique_ptr<float[]> chunkLoadTimes = std::make_unique<float[]>(mainWorld.getNumChunkLoaderThreads());
    std::fill(chunkLoadTimes.get(), chunkLoadTimes.get() + mainWorld.getNumChunkLoaderThreads(), 0.0f);
    int numThreadsBeingUsed = mainWorld.getNumChunkLoaderThreads();
    
    std::unique_ptr<std::thread[]> chunkLoaderThreads = std::make_unique<std::thread[]>(mainWorld.getNumChunkLoaderThreads() - 1);
    for (int8_t threadNum = 1; threadNum < mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        if (multiplayer) {
            chunkLoaderThreads[threadNum - 1] = std::thread(chunkLoaderThreadMultiplayer,
                std::ref(mainWorld), std::ref(networking), &running, threadNum,
                &numThreadsBeingUsed);
        }
        else {
            chunkLoaderThreads[threadNum - 1] = std::thread(chunkLoaderThreadSingleplayer,
            std::ref(mainWorld), &running, threadNum, &numThreadsBeingUsed);
        }
    }

    if (multiplayer) {
        auto lastMessage = std::chrono::steady_clock::now();
        while (running) {
            auto startTime = std::chrono::steady_clock::now();
            mainWorld.loadChunksAroundPlayerMultiplayer(0);
            auto currentTime = std::chrono::steady_clock::now();

            // Manage threads
            chunkLoadTimes[numThreadsBeingUsed - 1] = chunkLoadTimes[
                 - 1] * 0.9999f
                + std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - startTime).
                count() * 0.0001f;
            //manageThreads(chunkLoadTimes.get(), mainWorld.getNumChunkLoaderThreads(),
            //    &numThreadsBeingUsed);

            if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastMessage) > std::chrono::milliseconds(100)) {
                Packet<int, 3> payload(mainWorld.getClientID(), PacketType::ClientPosition, 3);
                payload[0] = mainPlayer.cameraBlockPosition[0];
                payload[1] = mainPlayer.cameraBlockPosition[1];
                payload[2] = mainPlayer.cameraBlockPosition[2];
                ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
                enet_peer_send(networking.getPeer(), 0, packet);
                lastMessage += std::chrono::milliseconds(100);
            }
        }
    }
    else {
        auto nextTick = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
        while (running) {
            mainWorld.loadChunksAroundPlayerSingleplayer(0);

            chunkLoadTimes[numThreadsBeingUsed - 1] -= 1.0f;

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= nextTick) {
                if (mainWorld.getTickNum() % 10 == 0) {
                    //manageThreads(chunkLoadTimes.get(), mainWorld.getNumChunkLoaderThreads(),
                    //    &numThreadsBeingUsed);
                    chunkLoadTimes[numThreadsBeingUsed - 1] = 1000.0f;
                }

                mainWorld.tick();
                nextTick += std::chrono::milliseconds(100);
            }
        }
    }
    chunkLoaderThreadsRunning[0] = false;

    for (int8_t threadNum = 1; threadNum < mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        chunkLoaderThreads[threadNum - 1].join();
        chunkLoaderThreadsRunning[threadNum] = false;
    }
    renderWorker.join();

    delete chunkLoaderThreadsRunning;

    if (multiplayer) {
        enet_peer_disconnect(networking.getPeer(), 0);
        ENetEvent event;
        while (enet_host_service(networking.getHost(), &event, 3000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
                case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnection succeeded!" << std::endl;
                break;
            }
        }

        enet_deinitialize();
    }

    return 0;
}