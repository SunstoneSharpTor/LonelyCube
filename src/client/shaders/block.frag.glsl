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

#version 450

layout (location = 0) in vec2 inTexCoord;
layout (location = 1) in float inSkyBrightness;
layout (location = 2) in float inBlockBrightness;
layout (location = 3) in float inVisibility;
layout (location = 0) out vec4 outColour;

layout (set = 0, binding = 0) uniform sampler2D blockTextures;
layout (rgba16f, set = 0, binding = 1) uniform image2D skyImage;

const vec4 BLOCK_LIGHT_COLOUR = vec4(1.0, 0.839, 0.631, 1.0);

void main() {
    vec4 skyColour = imageLoad(skyImage, ivec2(gl_FragCoord.xy));

    vec4 texColour = texture(blockTextures, inTexCoord);
    // if(texColour.a <= 252.4f/255.0f) {
    //     discard;
    // }
    vec4 texColourWithLight = texColour * inSkyBrightness;
    texColourWithLight += texColour * inBlockBrightness * BLOCK_LIGHT_COLOUR;
    texColourWithLight[3] = 1.0;
    outColour = mix(skyColour, texColourWithLight, inVisibility);
}
