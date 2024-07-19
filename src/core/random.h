/*
  Copyright (C) 2024 Bertie Cartwright

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

unsigned int PCG_Hash32(unsigned int input);

unsigned int PCG_Random32();

void PCG_SeedRandom32(unsigned long long seed);

void seedNoise();

//float simplexNoise1d(float x, float y);

float simplexNoise2d(float x, float y);

float simplexNoise2d(float x, float y, float* borderDistance);

void simplexNoiseGrad2d(float x, float y, float* value, float* gradient);

//float simplexNoise3d(float x, float y);