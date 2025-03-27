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

layout (rgba16f, set = 0, binding = 0) uniform writeonly image2D drawImage;

layout (push_constant, std430) uniform constants
{
    vec3 sunDir;
    float brightness;
    mat4 inverseViewProjection;
    vec3 sunGlowColour;
    float sunGlowAmount;
    vec2 renderSize;
};

void main()
{
    ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);

    vec2 pos;
    pos.x = (texelCoords.x * 2 - renderSize.x) / renderSize.x;  // Transforms to [-1.0, 1.0]
    pos.y = (texelCoords.y * 2 - renderSize.y) / renderSize.y;  // Transforms to [-1.0, 1.0]
    vec3 rayDir = normalize((inverseViewProjection * vec4(pos.x, pos.y, 1, 1)).xyz);

    float sunDistance = dot(rayDir, sunDir) + 1.0;//max(max(abs(rayDir.x - sunDir.x), abs(rayDir.y - sunDir.y)), abs(rayDir.z - sunDir.z));
    if (sunDistance > 1.9997) {//if (sunDistance < 0.025) {
        imageStore(drawImage, texelCoords, vec4(vec3(1.0, 0.7, 0.25) * brightness, 1.0));
    }
}
