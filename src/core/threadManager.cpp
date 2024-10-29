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

#include "core/threadManager.h"

#include "core/pch.h"

#include "core/resourceMonitor.h"

ThreadManager::ThreadManager(int numThreads, std::thread& threadToBeTimed) :
    m_numThreads(numThreads), m_numThreadsBeingUsed(1),
    m_lastTimePoint(std::chrono::steady_clock::now()),
    m_lastCPUTimePoint(getThreadCPUTime(threadToBeTimed)), m_threadToBeTimed(threadToBeTimed),
    m_threads(std::make_unique<std::thread[]>(numThreads)) {}

void ThreadManager::throttleThreads() {
    // Calculate how much time and CPU time has elapsed since the last call to this function
    auto currentTimePoint = std::chrono::steady_clock::now();
    double currentCPUTimePoint = getThreadCPUTime(m_threadToBeTimed);
    double deltaTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(
        currentTimePoint - m_lastTimePoint).count() / 1e6;
    double deltaCPUTime = currentCPUTimePoint - m_lastCPUTimePoint;
    m_lastTimePoint = currentTimePoint;
    m_lastCPUTimePoint = currentCPUTimePoint;

    if (currentCPUTimePoint == -1.0) {
        m_numThreadsBeingUsed = 1;
        return;
    }

    double fractionOfTimeRunning = deltaCPUTime / deltaTime;
    if (fractionOfTimeRunning > 0.95) {
        m_numThreadsBeingUsed = std::min(m_numThreads, m_numThreadsBeingUsed + 1);
    }
    if (fractionOfTimeRunning < 0.9) {
        m_numThreadsBeingUsed = std::max(1, m_numThreadsBeingUsed - 1);
    }
}

void ThreadManager::joinThreads()
{
    for (int8_t threadNum = 0; threadNum < m_numThreads - 1; threadNum++)
    {
        m_threads[threadNum].join();
    }
}