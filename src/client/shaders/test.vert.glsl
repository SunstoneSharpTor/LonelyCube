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

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec2 outUV;

struct Vertex
{
    vec3 position;
    float uv_x;
    vec4 colour;
    float uv_y;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout (push_constant) uniform constants
{
    mat4 mvp;
    VertexBuffer vertexBuffer;
} PushConstants;

void main() {
    Vertex vertex = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    gl_Position = PushConstants.mvp * vec4(vertex.position, 1.0f);
    outColour = vertex.colour;
    outUV.x = vertex.uv_x;
    outUV.y = vertex.uv_y;
}
