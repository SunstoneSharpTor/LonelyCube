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

#include "core/config.h"

#include "core/pch.h"

namespace lonelycube {

Config::Config(std::filesystem::path settingsPath) {
    // Set defaults
    m_renderDistance = 8;
    m_serverIP = "127.0.0.1";
    m_multiplayer = false;

    // Parse file
    std::string line, field, value;
    std::ifstream stream(settingsPath);
    while (std::getline(stream, line)) {
        line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
        std::stringstream lineStream(line);
        std::getline(lineStream, field, ':');
        std::getline(lineStream, value, ':');
        std::transform(field.begin(), field.end(), field.begin(),
            [](uint8_t c){ return std::tolower(c); });
        if (field == "renderdistance") {
            m_renderDistance = std::stoi(value);
        }
        if (field == "serveripaddress") {
            m_serverIP = value;
        }
        if (field == "multiplayer") {
            value.erase(std::remove_if(value.begin(), value.end(), isspace), value.end());
            std::transform(value.begin(), value.end(), value.begin(),
                [](uint8_t c){ return std::tolower(c); });
            if (value == "true") {
                m_multiplayer = true;
            }
            else {
                m_multiplayer = false;
            }
        }
    }
    stream.close();
}

}  // namespace lonelycube
