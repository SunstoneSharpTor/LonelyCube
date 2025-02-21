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

layout (local_size_x = 16, local_size_y = 16) in;

layout (rgba16f, set = 0, binding = 0) uniform image2D skyImage;
layout (rgba16f, set = 0, binding = 1) uniform image2D drawImage;

layout (push_constant, std430) uniform constants
{
    vec3 sunDir;
    float brightness;
    mat4 inverseViewProjection;
    vec3 sunGlowColour;
    float sunGlowAmount;
    vec2 renderSize;
};

const float PLANET_RADIUS = 100.0;
const float ATMOSPHERE_RADIUS = 120.0;
const vec3 SKY_TOP_COLOUR = vec3(0.15, 0.25, 0.94);
const vec3 HORIZON_COLOUR = vec3(0.35, 0.4, 0.75);

float distanceThroughAtmosphere(vec3 rayDir) {
    vec3 spereCentre = vec3(0, 0, 0);
    float spereRadius = ATMOSPHERE_RADIUS;
    vec3 rayOrigin = vec3(0, PLANET_RADIUS, 0);
    vec3 offset = rayOrigin - spereCentre;
    float b = 2 * dot(offset, rayDir);
    float c = dot(offset, offset) - spereRadius * spereRadius;
    float d = b * b - 4 * c;
    float s = sqrt(d);
    return (-b + s) / 2;
}

void main() {
    ivec2 texelCoords = ivec2(gl_GlobalInvocationID.xy);

    vec2 pos;
    pos.x = (texelCoords.x * 2 - renderSize.x) / renderSize.x;  // Transforms to [-1.0, 1.0]
    pos.y = (texelCoords.y * 2 - renderSize.y) / renderSize.y;  // Transforms to [-1.0, 1.0]
    vec3 rayDir = normalize((inverseViewProjection * vec4(pos.x, pos.y, 1, 1)).xyz);

    float rayDistanceThroughAtmosphere = min(distanceThroughAtmosphere(rayDir), ATMOSPHERE_RADIUS);
    vec3 value = SKY_TOP_COLOUR * (1.0f - rayDistanceThroughAtmosphere / 90.0);
    value += HORIZON_COLOUR * rayDistanceThroughAtmosphere / 90.0;
    float dotProd = dot(rayDir, sunDir) + 4.0;
    dotProd *= dotProd;
    dotProd *= dotProd;
    dotProd *= dotProd;
    float sunDistance = dotProd * 0.000003 * min(rayDistanceThroughAtmosphere -
        (ATMOSPHERE_RADIUS - PLANET_RADIUS) * 0.5, ATMOSPHERE_RADIUS * 0.5) / 75 * sunGlowAmount;
    value = value * (1.0 - sunDistance) + sunGlowColour * sunDistance;
    value *= brightness;

    imageStore(skyImage, texelCoords, vec4(value, 1.0));
    imageStore(drawImage, texelCoords, vec4(value, 1.0));
}
