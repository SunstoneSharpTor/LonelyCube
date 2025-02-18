/*
  Lonely Cube, a voxel game
  Copyright (C) g 2024-2025 Bertie Cartwright

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

#include "client/logicThread.h"
#include <enet/enet.h>

#include "client/clientNetworking.h"
#include "client/clientWorld.h"
#include "client/clientPlayer.h"
#include "core/constants.h"
#include "core/packet.h"
#include "core/threadManager.h"

namespace lonelycube::client {

static void chunkLoaderThreadSingleplayer(ClientWorld& mainWorld, bool& running, int8_t threadNum, int&
    numThreadsBeingUsed) {
    while (running) {
        while (threadNum >= numThreadsBeingUsed && running) {
            mainWorld.setThreadWaiting(threadNum, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            mainWorld.setThreadWaiting(threadNum, false);
        }
        mainWorld.loadChunksAroundPlayerSingleplayer(threadNum);
    }
}

static void chunkLoaderThreadMultiplayer(ClientWorld& mainWorld, ClientNetworking& networking, bool&
    running, int8_t threadNum, int& numThreadsBeingUsed) {
    int numNCalls = 0;
    while (running) {
        while (threadNum >= numThreadsBeingUsed && running) {
            mainWorld.setThreadWaiting(threadNum, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            mainWorld.setThreadWaiting(threadNum, false);
        }
        mainWorld.loadChunksAroundPlayerMultiplayer(threadNum);
        networking.receiveEvents(mainWorld);
    }
}

void LogicThread::go(bool& running)
{
    ThreadManager threadManager(m_mainWorld.getNumChunkLoaderThreads());
    for (int8_t threadNum = 1; threadNum < threadManager.getNumThreads(); threadNum++) {
        if (m_multiplayer) {
            threadManager.getThread(threadNum) = std::thread(chunkLoaderThreadMultiplayer,
                std::ref(m_mainWorld), std::ref(m_networking), std::ref(running), threadNum,
                std::ref(threadManager.getNumThreadsBeingUsed()));
        }
        else {
            threadManager.getThread(threadNum) = std::thread(chunkLoaderThreadSingleplayer,
                std::ref(m_mainWorld), std::ref(running), threadNum,
                std::ref(threadManager.getNumThreadsBeingUsed()));
        }
    }

    if (m_multiplayer) {
        auto nextTick = std::chrono::steady_clock::now() + std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
        while (running) {
            m_mainWorld.loadChunksAroundPlayerMultiplayer(0);
            m_networking.receiveEvents(m_mainWorld);

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= nextTick) {
                if (m_mainWorld.integratedServer.getTickNum() % 4 == 0)
                    threadManager.throttleThreads();

                // Send the server the player's position
                Packet<int, 3> payload(m_mainWorld.getClientID(), PacketType::ClientPosition, 3);
                payload[0] = m_mainPlayer.cameraBlockPosition[0];
                payload[1] = m_mainPlayer.cameraBlockPosition[1];
                payload[2] = m_mainPlayer.cameraBlockPosition[2];
                m_networking.getMutex().lock();
                ENetPacket* packet = enet_packet_create((const void*)(&payload), payload.getSize(), 0);
                enet_peer_send(m_networking.getPeer(), 1, packet);
                m_networking.getMutex().unlock();
                m_mainWorld.scheduleChunkRequest();

                nextTick += std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
            }
        }
    }
    else {
        auto nextTick = std::chrono::steady_clock::now() + std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
        while (running) {
            m_mainWorld.loadChunksAroundPlayerSingleplayer(0);

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= nextTick) {
                if (m_mainWorld.integratedServer.getTickNum() % 4 == 0)
                    threadManager.throttleThreads();
                m_mainWorld.integratedServer.tick();
                nextTick += std::chrono::nanoseconds(1000000000 / constants::TICKS_PER_SECOND);
            }
        }
    }
    m_chunkLoaderThreadsRunning[0] = false;

    threadManager.joinThreads();
    for (int8_t threadNum = 1; threadNum < m_mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        m_chunkLoaderThreadsRunning[threadNum] = false;
    }
}

}  // namespace lonelycube::client
