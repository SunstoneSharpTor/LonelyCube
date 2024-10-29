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

ThreadManager::ThreadManager(int numThreads) : m_numThreads(numThreads), m_numThreadsBeingUsed(1),
    m_threads(std::make_unique<std::thread[]>(numThreads))
{
    
}

void ThreadManager::joinThreads()
{
    for (int8_t threadNum = 0; threadNum < m_numThreads; threadNum++)
    {
        m_threads[threadNum].join();
    }
}