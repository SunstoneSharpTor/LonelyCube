#pragma once

unsigned int PCG_Hash32(unsigned int input);

unsigned int PCG_Random32();

void PCG_SeedRandom32(unsigned long long seed);

void seedNoise();

//float simplexNoise1d(float x, float y);

float simplexNoise2d(float x, float y);

float simplexNoise2d(float x, float y, float* borderDistance);

void simplexNoiseGrad2d(float x, float y, float* value, float* gradient);

//float simplexNoise3d(float x, float y);