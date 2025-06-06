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

#version 460

layout (location = 0) out vec4 outColour;

layout (set = 0, binding = 0) uniform sampler linearSampler;
layout (set = 1, binding = 0) uniform texture2D srcImage;

layout (push_constant, std430) uniform constants
{
    vec2 scale;
    vec2 offset;
};

void main() {
    outColour = vec4(
        texture(sampler2D(srcImage, linearSampler), gl_FragCoord.xy * scale + offset).rgb, 1.0
    );
}
