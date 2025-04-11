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

layout (location = 0) out vec2 outTexCoord;
layout (location = 1) out vec3 outColour;

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    float vertices[];
};

layout (push_constant, std430) uniform constants
{
    mat4 mvp;
    VertexBuffer vertexBuffer;
};

const uint vertexOrder[6] = uint[](0, 3, 1, 1, 3, 2);

void main()
{
    const uint vertexSize = 7;
    uint vertexIndex = (gl_VertexIndex / 6 * 4 + vertexOrder[gl_VertexIndex % 6]) * vertexSize;
    vec2 position = vec2(
        vertexBuffer.vertices[vertexIndex], vertexBuffer.vertices[vertexIndex + 1]
    );
    vec2 texCoord = vec2(
        vertexBuffer.vertices[vertexIndex + 2], vertexBuffer.vertices[vertexIndex + 3]
    );
    vec3 colour = vec3(
        vertexBuffer.vertices[vertexIndex + 4],
        vertexBuffer.vertices[vertexIndex + 5],
        vertexBuffer.vertices[vertexIndex + 6]
    );

    gl_Position = mvp * vec4(position, 0.0, 1.0);
    outTexCoord = texCoord;
    outColour = colour;
}
