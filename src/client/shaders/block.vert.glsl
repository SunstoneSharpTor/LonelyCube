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
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out float outSkyBrightness;
layout (location = 2) out float outBlockBrightness;
layout (location = 3) out float outVisibility;

struct Vertex
{
    vec3 position;
    vec2 texCoord;
    float skyLightLevel;
    float blockLightLevel;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    float vertices[];
};

layout (push_constant) uniform constants
{
    mat4 mvp;
    VertexBuffer vertexBuffer;
    vec3 cameraOffset;
    float renderDistance;
    float skyLightIntensity;
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
    vec2 texCoord = vec2(
        vertexBuffer.vertices[gl_VertexIndex * 7 + 3],
        vertexBuffer.vertices[gl_VertexIndex * 7 + 4]
    );
    float skyLightLevel = vertexBuffer.vertices[gl_VertexIndex * 7 + 5];
    float blockLightLevel = vertexBuffer.vertices[gl_VertexIndex * 7 + 6];

    float maxDarknessAmbientLight = min(0.001f, skyLightIntensity);

    gl_Position = mvp * position;
    outTexCoord = texCoord;
    float factor = skyLightLevel * skyLightLevel * skyLightLevel;
    //factor *= factor;
    outSkyBrightness = skyLightIntensity / (1.0 + (1.0 - skyLightLevel) * (1.0 - skyLightLevel) * 45.0)
        * factor + maxDarknessAmbientLight / (1.0 + (1.0 - skyLightLevel) * (1.0 - skyLightLevel) *
        45.0) * (1.0 - factor);
    outSkyBrightness = max(outSkyBrightness, minDarknessAmbientLight);
    factor = blockLightLevel * blockLightLevel;
    outBlockBrightness = blockLightIntensity / (1.0 + (1.0 - blockLightLevel) * (1.0 - blockLightLevel) * 45.0)
        * factor + maxDarknessAmbientLight / (1.0 + (1.0 - blockLightLevel) * (1.0 - blockLightLevel) *
        45.0) * (1.0 - factor);

    float distance = length(cameraOffset + position.xyz);
    float normalisedDistance = clamp(distance - renderDistance * fogStart, 0.0f, renderDistance
      * (1.0f - fogStart)) / (renderDistance * (1.0f - fogStart));
    outVisibility = normalisedDistance * fogDensity;
    outVisibility = exp(-outVisibility * outVisibility);
    outVisibility *= clamp((renderDistance - distance) / renderDistance, 0.0f, 0.04f) * 25.0f;
}
