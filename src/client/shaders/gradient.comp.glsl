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

layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba16f, set = 0, binding = 0) uniform image2D image;

void main()
{
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(image);

    if (texelCoord.x < size.x && texelCoord.y < size.y)
    {
        vec4 colour = vec4(0.0, 0.0, 0.0, 1.0);

        if (gl_LocalInvocationID.x != 0 && gl_LocalInvocationID.y != 0)
        {
            colour.x = float(texelCoord.x) / size.x;
            colour.y = float(texelCoord.y) / size.y;
        }

        imageStore(image, texelCoord, colour);
    }
}
