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
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out float outSkyBrightness;
layout (location = 2) out float outBlockBrightness;
layout (location = 3) out float outVisibility;

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    float vertices[];
};

layout (push_constant, std430) uniform constants
{
    mat4 mvp;
    vec3 playerSubBlockPos;
    float renderDistance;
    vec3 chunkCoordinates;
    float skyLightIntensity;
    VertexBuffer vertexBuffer;
};

const float fogStart = 0.4;
const float fogDensity = 1.85;
const float minDarknessAmbientLight = 0.00002;
const float blockLightIntensity = 0.04;

void main() {
    vec4 position = vec4(
        vertexBuffer.vertices[gl_VertexIndex * 7],
        vertexBuffer.vertices[gl_VertexIndex * 7 + 1],
        vertexBuffer.vertices[gl_VertexIndex * 7 + 2],
        1.0
    );
    position += vec4(chunkCoordinates, 0.0);
    vec2 texCoord = vec2(
        vertexBuffer.vertices[gl_VertexIndex * 7 + 3],
        vertexBuffer.vertices[gl_VertexIndex * 7 + 4]
    );
    float skyLightLevel = vertexBuffer.vertices[gl_VertexIndex * 7 + 5];
    float blockLightLevel = vertexBuffer.vertices[gl_VertexIndex * 7 + 6];

    float maxDarknessAmbientLight = min(0.001f, skyLightIntensity);

    gl_Position = mvp * position;
    gl_Position.z *= 0.996f;
    outTexCoord = texCoord;
    float factor = skyLightLevel * skyLightLevel * skyLightLevel;
    outSkyBrightness = mix(
        maxDarknessAmbientLight / (1.0 + (1.0 - skyLightLevel) * (1.0 - skyLightLevel) * 45.0),
        skyLightIntensity / (1.0 + (1.0 - skyLightLevel) * (1.0 - skyLightLevel) * 45.0),
        factor
    );
    outSkyBrightness = max(outSkyBrightness, minDarknessAmbientLight);

    factor = blockLightLevel * blockLightLevel;
    outBlockBrightness = mix(
        maxDarknessAmbientLight / (1.0 + (1.0 - blockLightLevel) * (1.0 - blockLightLevel) * 45.0),
        blockLightIntensity / (1.0 + (1.0 - blockLightLevel) * (1.0 - blockLightLevel) * 45.0),
        factor
    );

    vec3 toCameraVector = position.xyz - playerSubBlockPos;
    float distance = length(toCameraVector);
    float normalisedDistance = clamp(distance - renderDistance * fogStart, 0.0f, renderDistance
      * (1.0f - fogStart)) / (renderDistance * (1.0f - fogStart));
    outVisibility = normalisedDistance * fogDensity;
    outVisibility = exp(-outVisibility * outVisibility);
    outVisibility *= clamp((renderDistance - distance) / renderDistance, 0.0f, 0.04f) * 25.0f;
}
