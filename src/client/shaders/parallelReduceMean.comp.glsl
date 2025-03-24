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
#extension GL_EXT_buffer_reference : require
#extension GL_KHR_shader_subgroup_basic : require

layout (buffer_reference, std430) buffer NumbersBuffer
{
    float elements[];
};

layout (constant_id = 0) const uint subgroupSize = 8;

layout (local_size_x_id = 0) in;

layout (push_constant, std430) uniform constants
{
    NumbersBuffer inputNumbers;
    NumbersBuffer outputNumbers;
};

shared float sharedNumbers[subgroupSize];

void main()
{
    sharedNumbers[gl_LocalInvocationID.x] = inputNumbers.elements[gl_GlobalInvocationID.x];
    subgroupMemoryBarrier();

    for (uint s = subgroupSize / 2; s > 0; s >>= 1)
    {
        if (gl_LocalInvocationID.x < s)
        {
            sharedNumbers[gl_LocalInvocationID.x] = (
                sharedNumbers[gl_LocalInvocationID.x] + sharedNumbers[gl_LocalInvocationID.x + s]
            ) / 2;
        }
        subgroupMemoryBarrier();
    }

    if (subgroupElect())
    {
        outputNumbers.elements[gl_WorkGroupID.x] = sharedNumbers[0];
    }
}
