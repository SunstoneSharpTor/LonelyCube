#pragma once

unsigned int PCG_Hash(unsigned int input);

//float simplexNoise1d(float x, float y);

float simplexNoise2d(float x, float y);

float simplexNoise2d(float x, float y, float* borderDistance);

void simplexNoiseGrad2d(float x, float y, float* value, float* gradient);

//float simplexNoise3d(float x, float y);