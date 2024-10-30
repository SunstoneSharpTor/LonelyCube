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

#include "resourceMonitor.h"
#include "pch.h"

static float calculateCPULoad(uint64_t idleTicks, uint64_t totalTicks)
{
   static uint64_t _previousTotalTicks = 0;
   static uint64_t _previousIdleTicks = 0;

   uint64_t totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
   uint64_t idleTicksSinceLastTime  = idleTicks - _previousIdleTicks;

   float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);

   _previousTotalTicks = totalTicks;
   _previousIdleTicks  = idleTicks;
   return ret;
}

#ifdef _WIN32
#include <windows.h>

static uint64_t FileTimeToInt64(const FILETIME & ft)
{
    return (((uint64_t)(ft.dwHighDateTime)) <<32 ) | ((uint64_t)ft.dwLowDateTime);
}

float getCPULoad()
{
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
    {
        return calculateCPULoad(FileTimeToInt64(idleTime),
            FileTimeToInt64(kernelTime)+FileTimeToInt64(userTime));
    }
    else
    {
        return -1.0f;
    }
}

#elif __linux__
    float getCPULoad() {
        std::ifstream file("/proc/stat");
        std::string line;
        if (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string cpu;
            uint64_t user, nice, system, idle;
            iss >> cpu >> user >> nice >> system >> idle;
            return calculateCPULoad(idle, user + nice + system);
        }
        return -1.0f;
    }
#endif