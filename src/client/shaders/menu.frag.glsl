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

#version 460

layout (location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 colour;

layout (set = 0, binding = 0) uniform sampler uiSampler;
layout (set = 1, binding = 0) uniform texture2D menuTexture;

void main() {
    vec4 texColour = texture(sampler2D(menuTexture, uiSampler), inTexCoord);
    if(texColour.a == 0.0) {
        discard;
    }
    colour = texColour;
};
