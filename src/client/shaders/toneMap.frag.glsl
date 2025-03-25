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

layout (buffer_reference, std430) buffer LuminanceBuffer
{
    float luminance[];
};

layout (location = 0) out vec4 outColour;

layout (set = 0, binding = 0) uniform sampler2D drawImage;

layout (push_constant, std430) uniform constants
{
    vec2 inverseDrawImageSize;
    LuminanceBuffer luminanceBuffer;
};

//

//https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
/*
=================================================================================================

  Baking Lab
  by MJP and David Neubelt
  http://mynameismjp.wordpress.com/

  All code licensed under the MIT license

=================================================================================================
 The code in this file was originally written by Stephen Hill (@self_shadow), who deserves all
 credit for coming up with this fit and implementing it. Buy him a beer next time you see him. :)
*/

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
mat3x3 ACESInputMat = mat3x3
(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
mat3x3 ACESOutputMat = mat3x3
(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = transpose(ACESInputMat) * color;
    // Apply RRT and ODT
    color = RRTAndODTFit(color);
    color = transpose(ACESOutputMat) * color;
    color = clamp(color, 0, 1);
    return color;
}

vec3 ACESFilmSimple(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0, 1);

}


// https://advances.realtimerendering.com/s2021/jpatry_advances2021/index.html
// https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.40.9608&rep=rep1&type=pdf
vec4 rgb_to_lmsr(vec3 c)
{
    const mat4x3 m = mat4x3(
        0.31670331, 0.70299344, 0.08120592, 
        0.10129085, 0.72118661, 0.12041039, 
        0.01451538, 0.05643031, 0.53416779, 
        0.01724063, 0.60147464, 0.40056206);
    return c * m;
}
vec3 lms_to_rgb(vec3 c)
{
    const mat3 m = mat3(
         4.57829597, -4.48749114,  0.31554848, 
        -0.63342362,  2.03236026, -0.36183302, 
        -0.05749394, -0.09275939,  1.90172089);
    return c * m;
}

void main() {
    vec3 hdrColour = texture(drawImage, vec2(gl_FragCoord.xy) * inverseDrawImageSize).rgb;

    // Exposure tone mapping
    // vec3 mapped = vec3(1.0) - exp(-hdrColour * exposure);
    float exposure = luminanceBuffer.luminance[0];
    vec3 mapped = ACESFitted(hdrColour * exposure);

    // // Gamma correction
    // const float gamma = 2.2;
    // mapped = pow(mapped, vec3(1.0 / gamma));

    outColour = vec4(mapped, 1.0);
}
