/*
  Lonely Cube, a voxel game
  Copyright (C) 2024-2025 Bertie Cartwright

  Lonely Cube is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Lonely Cube is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "client/gui/menu.h"

namespace lonelycube::client {

Menu::Menu(const glm::ivec2 windowDimensions)
{
    resize(windowDimensions);
}

void Menu::update(glm::ivec2 cursorPos)
{
    for (auto& element : m_elements)
    {
        if (element.type == Button)
        {
            glm::ivec2 position = element.buttonData.screenAlignment * m_windowDimensions +
                glm::vec2(element.buttonData.pixelOffset * m_scale);
            glm::ivec2 size(element.buttonData.width * m_scale, 14.0f * m_scale);
            element.buttonData.mouseOver = cursorPos.x >= position.x
                                        && cursorPos.x < position.x + size.x
                                        && cursorPos.y >= position.y
                                        && cursorPos.y < position.y + size.y;
        }
    }
}

void Menu::resize(const glm::ivec2 windowDimensions)
{
    m_windowDimensions = windowDimensions;
}

std::size_t Menu::addButton(
    int width, glm::vec2 screenAlignment, glm::ivec2 offset, const std::string& text
) {
    std::size_t index = m_elements.size();
    m_elements.push_back({ Button, { false, width, screenAlignment, offset}, text });

    return index;
}

}  // namespace lonelycube::client
