#pragma once

#include "renderer.h"
#include "vertexBuffer.h"
#include "indexBuffer.h"
#include "vertexArray.h"
#include "shader.h"
#include "texture.h"
#include "camera.h"
#include "chunk.h"

#include <chrono>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <SDL2/SDL.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

class world {
private:
	unsigned short m_renderDistance;
	unsigned short m_renderDiameter;
	unsigned int m_numChunks; //stores the number of chunks as if they were arranged in a grid
	unsigned int m_numActualChunks; //stores the number of chunks as if they were arranged in a sphere
	chunk* m_chunks;
	int m_playerChunkPosition[3];
	int m_newPlayerChunkPosition[3];
	int m_updatingPlayerChunkPosition[3];
	bool* m_loadedChunks;
	bool* m_loadingChunks;
	float* m_chunkDistances;
	bool* m_meshUpdates; //stores chunks that have to have their meshes rebuilt after a block update
	int m_numMeshUpdates;
	//stores the location of each chunk in the array m_chunks, ordered by chunk number
	unsigned int* m_chunkArrayIndices;
	int m_neighbouringChunkNumberOffets[6];
	int m_neighbouringChunkIncludingDiaganalOffsets[27];
	worldInfo m_worldInfo;
	char m_numChunkLoadingThreads;
	unsigned int m_numMeshesUnloaded;
	unsigned int m_numChunksUnloaded;
	bool m_renderingFrame;
	float m_meshedChunksDistance;
	float m_fogDistance;
	double m_timeByDTs;

	//mouse polling info
	std::chrono::time_point<std::chrono::_V2::steady_clock, std::chrono::_V2::steady_clock::duration> m_startTime;
	double* m_lastMousePoll;
	bool* m_playing;
	bool* m_lastPlaying;
	float* m_yaw;
	float* m_pitch;
	int* m_lastMousePos;
	camera* m_viewCamera;
	SDL_Window* m_window;
	int* m_windowDimensions;

	std::vector<vertexArray*> m_chunkVertexArrays;
	std::vector<vertexBuffer*> m_chunkVertexBuffers;
	std::vector<indexBuffer*> m_chunkIndexBuffers;
	std::vector<vertexArray*> m_chunkWaterVertexArrays;
	std::vector<vertexBuffer*> m_chunkWaterVertexBuffers;
	std::vector<indexBuffer*> m_chunkWaterIndexBuffers;
	vertexBuffer* m_emptyVertexBuffer;
	indexBuffer* m_emptyIndexBuffer;
	vertexArray* m_emptyVertexArray;
	//stores the positions of the loaded chunks in the array m_chunks
	std::vector<unsigned int> m_meshedChunkArrayIndices;
	std::vector<unsigned int> m_unmeshedChunkArrayIndices;
	std::vector<unsigned int> m_beingMeshedChunkArrayIndices;

	//mesh building data - this is stored at class-level because it allows it to be
	//accessed from multiple threads
	unsigned int* m_numChunkVertices; //array to allow for each mesh-building thread to have its own value
	unsigned int* m_numChunkWaterVertices; //array to allow for each mesh-building thread to have its own value
	unsigned int* m_numChunkIndices; //array to allow for each mesh-building thread to have its own value
	unsigned int* m_numChunkWaterIndices; //array to allow for each mesh-building thread to have its own value
	unsigned int* m_chunkVectorIndex; //array to allow for each mesh-building thread to have its own value
	float** m_chunkVertices; //2d array to allow for each mesh-building thread to have its own array
	unsigned int** m_chunkIndices; //2d array to allow for each mesh-building thread to have its own array
	float** m_chunkWaterVertices; //2d array to allow for each mesh-building thread to have its own array
	unsigned int** m_chunkWaterIndices; //2d array to allow for each mesh-building thread to have its own array

	//communication
	std::mutex* m_chunkMeshReadyMtx;
	std::condition_variable* m_chunkMeshReadyCV;
	std::mutex m_relableNeededMtx;
	std::condition_variable m_relableNeededCV;
	std::mutex m_accessingArrIndicesVectorsMtx;
	std::mutex m_renderThreadWaitingForArrIndicesVectorsMtx;
	bool m_renderThreadWaitingForArrIndicesVectors;
	bool* m_chunkMeshReady;
	bool m_relableNeeded;
	bool* m_relableOccurred;
	bool* m_threadWaiting;
	int m_mouseCalls;
	int m_numRelights;

	//chunk number is a number assigned to each chunk within render distance (between
	//0 and m_numChunks) that allows the chunks to be represented in a 1d array
	unsigned int getChunkNumber(int* chunkCoords, int* playerCoords);

	void getChunkCoords(int* chunkCoords, unsigned int chunkNumber, int* playerCoords);
	void loadChunk(unsigned int chunkArrayIndex, int* chunkCoords, char threadNum);
	void unloadChunk(unsigned int chunkVectorIndex);
	void unloadMesh(unsigned int chunkVectorIndex);
	bool chunkHasNeighbours(unsigned int chunkArrayIndex);
	void addChunkMesh(unsigned int chunkVectorIndex, char threadNum);
	void uploadChunkMesh(char threadNum);
	void unloadAndRelableChunks();
	void relightChunksAroundBlock(const int* blockCoords, std::vector<unsigned int>* relitChunks);

public:
	world(unsigned short renderDistance);
	void renderChunks(renderer mainRenderer, shader& blockShader, shader& waterShader, glm::mat4 viewMatrix, glm::mat4 projMatrix, int* playerBlockPosition, float aspectRatio, float fov, double DT);
	void loadChunksAroundPlayer(char threadNum);
	void buildMeshesForNewChunksWithNeighbours(char threadNum);
	unsigned char shootRay(glm::vec3 startSubBlockPos, int* startBlockPosition, glm::vec3 direction, int* breakBlockCoords, int* placeBlockCoords);
	void replaceBlock(int* blockCoords, unsigned short blockType);
	unsigned short getBlock(int* blockCoords);
	worldInfo getWorldInfo();
	void doRenderThreadJobs();
	void relableChunksIfNeeded();
	void updatePlayerPos(float playerX, float playerY, float playerZ);
	void initPlayerPos(float playerX, float playerY, float playerZ);
	char getNumChunkLoaderThreads();
	void setMouseData(double* lastMousePoll,
					  bool* playing,
					  bool* lastPlaying,
					  float* yaw,
					  float* pitch,
					  int* lastMousePos,
					  camera* viewCamera,
					  SDL_Window* window,
					  int* windowDimensions);
	void processMouseInput();
};