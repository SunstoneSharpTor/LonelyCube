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

#pragma once

#include "pch.h"
#include <thread>

class ThreadManager
{
    private:
        int m_numThreads;
        int m_numThreadsBeingUsed;
        std::unique_ptr<std::thread[]> m_threads;
    
    public:
        ThreadManager(int numThreads);
        void joinThreads();
        
        std::thread& getThread(int threadNum)
        {
            return m_threads[threadNum];
        }
        int getNumThreads()
        {
            return m_numThreads;
        }
        int& getNumThreadsBeingUsed()
        {
            return m_numThreadsBeingUsed;
        }
};