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
layout (set = 0, binding = 1) uniform sampler2D skyTexture;

const vec3 BLOCK_LIGHT_COLOUR = vec3(1.0, 0.839, 0.631);

void main() {
    vec4 skyColour = texture(skyTexture, ivec2(gl_FragCoord.xy) / vec2(textureSize(skyTexture, 0)));

    vec4 texColour = texture(blockTextures, inTexCoord);
    if(texColour.a <= 252.4f/255.0f) {
        discard;
    }
    vec3 texColourWithLight = texColour.rgb * inSkyBrightness;
    texColourWithLight += texColour.rgb * inBlockBrightness * BLOCK_LIGHT_COLOUR;
    outColour = vec4(mix(skyColour.rgb, texColourWithLight, inVisibility), 1.0);
}
