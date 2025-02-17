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

layout (rgba16f, set = 0, binding = 0) uniform image2D swapchainImage;
layout (set = 0, binding = 1) uniform sampler2D drawImage;

layout (push_constant, std430) uniform constants
{
    float exposure;
};

void main() {
    ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);
    vec3 hdrColour = texture(drawImage, texelCoords / vec2(textureSize(drawImage, 0))).rgb;

    // Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColour * exposure);

    // Gamma correction
    const float gamma = 2.2;
    mapped = pow(mapped, vec3(1.0 / gamma));

    imageStore(swapchainImage, texelCoords, vec4(mapped, 1.0));
}
