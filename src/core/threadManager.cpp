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

#include "core/threadManager.h"

#include "core/pch.h"

#include "core/resourceMonitor.h"

namespace lonelycube {

ThreadManager::ThreadManager(int numThreads) :
    m_numThreads(numThreads), m_numThreadsBeingUsed(1),
    m_numSystemThreads(std::thread::hardware_concurrency()),
    m_threads(std::make_unique<std::thread[]>(numThreads)) {}

void ThreadManager::throttleThreads() {
    float CPULoad = getCPULoad();
    if (CPULoad == -1.0) {
        m_numThreadsBeingUsed = 1;
        return;
    }

    if (CPULoad > 0.995f) {
        m_numThreadsBeingUsed = std::max(1, m_numThreadsBeingUsed - 1);
    }
    if (CPULoad < std::min(0.90f, 1.0f - 1.0f / m_numSystemThreads)) {
        m_numThreadsBeingUsed = std::min(m_numThreads, m_numThreadsBeingUsed + 1);
    }
    // std::cout << CPULoad << " cpu load\n";
    // std::cout << m_numThreadsBeingUsed << " threads\n";
}

void ThreadManager::joinThreads()
{
    for (int8_t threadNum = 0; threadNum < m_numThreads - 1; threadNum++)
    {
        m_threads[threadNum].join();
    }
}

}  // namespace lonelycube
