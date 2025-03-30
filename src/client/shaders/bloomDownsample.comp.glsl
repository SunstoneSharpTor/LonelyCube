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

layout (push_constant, std430) uniform constants
{
    vec2 srcTexelSize;
    vec2 srcRenderSize;
    vec2 dstTexelSize;
    float strength;
};

layout (local_size_x = 16, local_size_y = 16) in;

layout (set = 0, binding = 0) uniform sampler linearSampler;
layout (set = 1, binding = 0) uniform texture2D srcTexture;
layout (rgba16f, set = 1, binding = 1) uniform writeonly image2D dstImage;

float sampleWithinSrcImage(vec2 samplePos)
{
    return float(samplePos.x > 0
              && samplePos.y > 0
              && samplePos.x < srcRenderSize.x
              && samplePos.y < srcRenderSize.y);
}

void main()
{
    const vec2 texCoord = (gl_GlobalInvocationID.xy + vec2(0.5, 0.5)) * dstTexelSize;
    const float x = srcTexelSize.x;
    const float y = srcTexelSize.y;

    // a---b---c
    // --d---e--
    // f---g---h
    // --i---j--
    // k---l---m
    vec2 samplePos = vec2(texCoord.x - 2 * x, texCoord.y + 2 * y);
    vec3 a = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    vec3 downsample = a * sampleWithinSrcImage(samplePos) * 0.03125;
    samplePos = vec2(texCoord.x, texCoord.y + 2 * y);
    vec3 b = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += b * sampleWithinSrcImage(samplePos) * 0.0625;
    samplePos = vec2(texCoord.x + 2 * x, texCoord.y + 2 * y);
    vec3 c = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += c * sampleWithinSrcImage(samplePos) * 0.03125;

    samplePos = vec2(texCoord.x - x, texCoord.y + y);
    vec3 d = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += d * sampleWithinSrcImage(samplePos) * 0.125;
    samplePos = vec2(texCoord.x + x, texCoord.y + y);
    vec3 e = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += e * sampleWithinSrcImage(samplePos) * 0.125;

    samplePos = vec2(texCoord.x - 2 * x, texCoord.y);
    vec3 f = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += f * sampleWithinSrcImage(samplePos) * 0.0625;
    samplePos = vec2(texCoord.x, texCoord.y);
    vec3 g = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += g * sampleWithinSrcImage(samplePos) * 0.125;
    samplePos = vec2(texCoord.x + 2 * x, texCoord.y);
    vec3 h = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += h * sampleWithinSrcImage(samplePos) * 0.0625;

    samplePos = vec2(texCoord.x - x, texCoord.y - y);
    vec3 i = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += i * sampleWithinSrcImage(samplePos) * 0.125;
    samplePos = vec2(texCoord.x + x, texCoord.y - y);
    vec3 j = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += j * sampleWithinSrcImage(samplePos) * 0.125;

    samplePos = vec2(texCoord.x - 2 * x, texCoord.y - 2 * y);
    vec3 k = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += k * sampleWithinSrcImage(samplePos) * 0.03125;
    samplePos = vec2(texCoord.x, texCoord.y - 2 * y);
    vec3 l = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += l * sampleWithinSrcImage(samplePos) * 0.0625;
    samplePos = vec2(texCoord.x + 2 * x, texCoord.y - 2 * y);
    vec3 m = texture(sampler2D(srcTexture, linearSampler), samplePos).rgb;
    downsample += m * sampleWithinSrcImage(samplePos) * 0.03125;

    imageStore(dstImage, ivec2(gl_GlobalInvocationID.xy), vec4(downsample * strength, 1.0));
}
