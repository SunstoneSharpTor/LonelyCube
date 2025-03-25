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

layout (buffer_reference, std430) buffer LuminanceBuffer
{
    float values[];
};

layout (constant_id = 0) const uint numLuminanceElements = 1;

layout (local_size_x = 1) in;

layout (push_constant, std430) uniform constants
{
    LuminanceBuffer luminanceBuffer;
    LuminanceBuffer exposureBuffer;
    uint numTicks;
};

void main() {
    float averageLuminance = 0.0;
    for (int i = 0; i < numLuminanceElements; i++)
    {
        averageLuminance += luminanceBuffer.values[i];
    }
    averageLuminance /= numLuminanceElements;

    float exposure = exposureBuffer.values[0];
    float targetExposure = clamp(0.27 / exp(averageLuminance), 0.01, 2000.0);
    for (int i = 0; i < numTicks; i++)
    {
        float adjustmentSpeed = min(log(abs(targetExposure - exposure) + 1.0) * 0.025, 1.0);
        exposure = exposure * (1.0 - adjustmentSpeed) + targetExposure * adjustmentSpeed;
    }

    exposureBuffer.values[0] = exposure;
}
