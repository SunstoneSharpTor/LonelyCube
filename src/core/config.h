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

#pragma once

#include "pch.h"

namespace lonelycube {

class Config {
private:
    uint16_t m_renderDistance;
    std::string m_serverIP;
    bool m_multiplayer;
public:
    Config(std::filesystem::path settingsPath);
    uint16_t getRenderDistance() const {
        return m_renderDistance;
    }
    std::string& getServerIP() {
        return m_serverIP;
    }
    bool getMultiplayer() const {
        return m_multiplayer;
    }
};

}  // namespace lonelycube
