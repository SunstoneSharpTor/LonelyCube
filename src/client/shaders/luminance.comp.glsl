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
    float luminance[];
};

layout (local_size_x = 16, local_size_y = 16) in;

layout (set = 0, binding = 0) uniform sampler2D srcTexture;

layout (push_constant, std430) uniform constants
{
    LuminanceBuffer luminanceBuffer;
    vec2 renderAreaFraction;
    int luminanceImageSize;
};

void main() {
    ivec2 luminanceTexelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 srcTextureCoord = (vec2(luminanceTexelCoord) + vec2(0.5, 0.5)) * 
        renderAreaFraction / luminanceImageSize;

    vec4 srcColour = texture(srcTexture, srcTextureCoord);
    float luminance = log(
        (srcColour.r * 0.3) + (srcColour.g * 0.59) + (srcColour.b * 0.11) + 0.00001
    );
    // Add 0.00001 to the luminance to prevent taking the log of 0
    luminanceBuffer.luminance[luminanceTexelCoord.y * luminanceImageSize + luminanceTexelCoord.x] =
        luminance + 0.00001;
}
