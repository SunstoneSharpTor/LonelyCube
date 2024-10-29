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

#ifdef _WIN32
#include <windows.h>

double getThreadCPUTime(std::thread& thread) {
    //HANDLE threadHandle = thread.native_handle();
    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (GetThreadTimes(GetCurrentThread(), &creationTime, &exitTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER kernel, user;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;
        return (kernel.QuadPart + user.QuadPart) / 1e7;  // Convert to seconds
    }
    return -1.0;
}
#elif __linux__
#include <time.h>

double getThreadCPUTime(std::thread& thread) {
    auto thread_id = thread.native_handle();
    struct timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) == 0) {
        return ts.tv_sec + ts.tv_nsec / 1e9;  // Convert to seconds
    }
    return -1.0;
}
#endif