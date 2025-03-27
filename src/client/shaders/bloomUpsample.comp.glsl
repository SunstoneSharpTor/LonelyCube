/*
  Lonely Cube, a voxel game
  Copyright (C) 2024-2025 Bertie Cartwright

  Lonely Cube is free software: filterRadiusou can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at filterRadiusour option) any later version.

  Lonely Cube is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  filterRadiusou should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#version 460

layout (push_constant, std430) uniform constants
{
    vec2 dstTexelSize;
    float filterRadius;
};

layout (local_size_x = 16, local_size_y = 16) in;

layout (set = 0, binding = 0) uniform sampler linearSampler;
layout (set = 1, binding = 0) uniform texture2D srcTexture;
layout (rgba16f, set = 1, binding = 1) uniform writeonly image2D dstImage;

void main() {
    vec2 texCoord = (gl_GlobalInvocationID.xy + vec2(0.5, 0.5)) * dstTexelSize;

    vec3 a = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x - filterRadius, texCoord.y + filterRadius)).rgb;
    vec3 b = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x, texCoord.y + filterRadius)).rgb;
    vec3 c = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x + filterRadius, texCoord.y + filterRadius)).rgb;

    vec3 d = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x - filterRadius, texCoord.y)).rgb;
    vec3 e = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x, texCoord.y)).rgb;
    vec3 f = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x + filterRadius, texCoord.y)).rgb;

    vec3 g = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x - filterRadius, texCoord.y - filterRadius)).rgb;
    vec3 h = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x, texCoord.y - filterRadius)).rgb;
    vec3 i = texture(sampler2D(srcTexture, linearSampler),
                     vec2(texCoord.x + filterRadius, texCoord.y - filterRadius)).rgb;

    vec3 upsample = e * 0.25;
    upsample += (b + d + f + h) * 0.125;
    upsample += (a + c + g + i) * 0.0625;

    imageStore(dstImage, ivec2(gl_GlobalInvocationID.xy), vec4(upsample, 1.0));
}
