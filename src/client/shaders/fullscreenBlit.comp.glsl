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

layout (local_size_x = 16, local_size_y = 16) in;

layout (set = 0, binding = 0) uniform sampler linearSampler;
layout (set = 1, binding = 0) uniform texture2D srcImage;
layout (rgba16f, set = 1, binding = 1) uniform writeonly image2D dstImage;

void main() {
    vec2 textureCoords = gl_GlobalInvocationID.xy / imageSize(dstImage);
    vec4 colour = texture(sampler2D(srcImage, linearSampler), textureCoords);
    imageStore(dstImage, ivec2(gl_GlobalInvocationID.xy), colour);
}
