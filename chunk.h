#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>

#include "constants.h"

class chunk;

struct worldInfo {
	chunk* worldChunks;
	unsigned int* chunkArrayIndices;
	int* playerChunkPosition;
	unsigned int renderDistance;
	unsigned int renderDiameter;
	int* numRelights;
	unsigned long long seed;
};

class chunk {
private:
	unsigned char* m_blocks;
	unsigned char* m_skyLight;
	bool m_singleBlockType;
	bool m_singleSkyLightVal;
	bool m_skyLightUpToDate;
	int m_position[3]; //the chunks position in chunk coordinates (multiply by chunk size to get world coordinates)
	worldInfo m_worldInfo;
	bool m_calculatingSkylight;
	std::mutex m_accessingSkylightMtx;
	std::condition_variable m_accessingSkylightCV;
	static std::mutex s_checkingNeighbouringRelights;

	static const short m_neighbouringBlocks[6];
	static const short m_neighbouringBlocksX[6];
	static const short m_neighbouringBlocksY[6];
	static const short m_neighbouringBlocksZ[6];
	static const float m_cubeTextureCoordinates[48];
	static const float m_xTextureCoordinates[32];
	static const short m_blockIdToTextureNum[48];
	static const int m_neighbouringChunkBlockOffsets[6];
	static const short m_adjacentBlocksToFaceOffests[48];
	static const short m_adjacentBlocksToFaceOffestsX[48];
	static const short m_adjacentBlocksToFaceOffestsY[48];
	static const short m_adjacentBlocksToFaceOffestsZ[48];

	static const float m_continentalnessNoiseVals[10];
	static const float m_continentalnessTerrainHeights[10];
	static const float m_erosionNoiseVals[10];
	static const float m_erosionTerrainHeights[10];
	static const float m_smallErosionNoiseVals[10];
	static const float m_smallErosionTerrainHeights[10];
	static const float m_peaksAndValleysNoiseVals[10];
	static const float m_peaksAndValleysTerrainHeights[10];
	static const float m_bumpsNoiseVals[10];
	static const float m_bumpsTerrainHeights[10];

	static const float m_beachTerrainHeights[10];
	static const float m_beachNoiseVals[10];

	void addFaceToMesh(float* vertices, unsigned int* numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices, unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices, unsigned int block, short neighbouringBlock);

	void getTextureCoordinates(float* coords, short textureNum);

	void findBlockCoordsInChunk(float* blockPos, unsigned int block);

	void findBlockCoordsInChunk(unsigned short* blockPos, unsigned int block);

	void findBlockCoordsInWorld(int* blockPos, unsigned int block);

	unsigned char getWorldBlock(int* blockCoords);

	unsigned char getWorldSkyLight(int* blockCoords);

	void generateHeightMap(int* heightMap, int minX, int minZ, int size);

	void calculateFractalNoiseOctaves(float* noiseArray, int minX, int minZ, int size, int numOctaves, float scale);

	inline void setSkyLight(unsigned int block, unsigned char value) {
		bool oddBlockNum = block % 2;
		bool evenBlockNum = !oddBlockNum;
		m_skyLight[block / 2] &= 0b00001111 << (4 * evenBlockNum);
		m_skyLight[block / 2] |= value << (4 * oddBlockNum);
	}

public:
	bool inUse;

	chunk(int x, int y, int z, worldInfo wio);

	chunk(worldInfo wio);

	chunk();

	void recreate(int x, int y, int z);

	void setWorldInfo(worldInfo wio);

	void generateTerrain();

	void buildMesh(float* vertices, unsigned int* numVertices, unsigned int* indices, unsigned int* numIndices, float* waterVertices, unsigned int* numWaterVertices, unsigned int* waterIndices, unsigned int* numWaterIndices, unsigned int* neighbouringChunkIndices);

	void getChunkPosition(int* coordinates);

	void unload();

	inline char getBlock(unsigned int block) {
		return m_blocks[block * (!m_singleBlockType)];
	}

	inline char getSkyLight(unsigned int block) {
		return (m_skyLight[block / 2] >> (4 * (block % 2))) & 0b1111;
	}

	void setBlock(unsigned int block, unsigned short blockType);

	unsigned int getBlockNumber(unsigned int* blockCoords);

	void calculateSkyLight(unsigned int* neighbouringChunkIndices, bool* neighbouringChunksToBeRelit);

	void clearSkyLight();

	inline void setSkyLightToBeOutdated() {
		m_skyLightUpToDate = false;
	}

	inline bool skyBeingRelit() {
		return m_calculatingSkylight;
	}
};