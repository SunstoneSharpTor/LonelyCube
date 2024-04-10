#pragma once

static unsigned int PCG_Hash(unsigned int input) {
	unsigned int state = input * 747796405u + 2891336453u;
	unsigned int word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}