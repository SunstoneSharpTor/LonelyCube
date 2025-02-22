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

struct Vertex
{
    vec3 position;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer
{
    float vertices[];
};

layout (push_constant, std430) uniform constants
{
    mat4 mvp;
    VertexBuffer vertexBuffer;
};

void main() {
    vec4 position = vec4(
        vertexBuffer.vertices[gl_VertexIndex * 3],
        vertexBuffer.vertices[gl_VertexIndex * 3 + 1],
        vertexBuffer.vertices[gl_VertexIndex * 3 + 2],
        1.0
    );
    gl_Position = mvp * position;
}
