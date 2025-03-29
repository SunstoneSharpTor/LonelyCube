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
#extension GL_EXT_samplerless_texture_functions : require

layout (location = 0) out vec4 outColour;

layout (set = 0, binding = 0) uniform sampler imageSampler;
layout (set = 0, binding = 1) uniform texture2D image;

void main()
{
    outColour = texture(
        sampler2D(image, imageSampler), vec2(gl_FragCoord.xy) / vec2(textureSize(image, 0))
    );
}
