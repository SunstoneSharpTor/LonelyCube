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

layout (set = 0, binding = 0) uniform sampler2D drawImage;

layout (push_constant, std430) uniform constants
{
    vec2 inverseDrawImageSize;
    float exposure;
};

void main() {
    vec3 hdrColour = texture(drawImage, vec2(gl_FragCoord.xy) * inverseDrawImageSize).rgb;

    // Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColour * exposure);

    // // Gamma correction
    // const float gamma = 2.2;
    // mapped = pow(mapped, vec3(1.0 / gamma));

    outColour = vec4(mapped, 1.0);
}
