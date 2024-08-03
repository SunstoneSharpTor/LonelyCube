/*
  Lonely Cube, a voxel game
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

#include "compression.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/packet.h"

void Compression::compressChunk(Packet<unsigned char,
    9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE>& compressedChunk,
    Chunk& chunk) {
    int chunkPosition[3];
    chunk.getChunkPosition(chunkPosition);
    unsigned int packetIndex = 0;
    for (int i = 0; i < 3; i++) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            compressedChunk[packetIndex] = chunkPosition[i] >> shift;
            packetIndex++;
        }
    }
    // Add blocks
    unsigned char currentBlock = chunk.getBlock(0);
    unsigned short count = 0;
    unsigned char nextBlock;
    for (unsigned int block = 1; block < constants::CHUNK_SIZE
                                       * constants::CHUNK_SIZE
                                       * constants::CHUNK_SIZE; block++) {
        nextBlock = chunk.getBlock(block);
        if ((nextBlock == currentBlock) && (count < 65535)) {
            count++;
        }
        else {
            compressedChunk[packetIndex] = currentBlock;
            compressedChunk[packetIndex + 1] = count >> 8;
            compressedChunk[packetIndex + 2] = count;
            packetIndex += 3;
            currentBlock = nextBlock;
            count = 0;
        }
    }
    compressedChunk[packetIndex] = currentBlock;
    compressedChunk[packetIndex + 1] = count >> 8;
    compressedChunk[packetIndex + 2] = count;
    packetIndex += 3;
    // Add skylight
    currentBlock = chunk.getSkyLight(0);
    count = 0;
    for (unsigned int block = 1; block < constants::CHUNK_SIZE
                                       * constants::CHUNK_SIZE
                                       * constants::CHUNK_SIZE; block++) {
        nextBlock = chunk.getSkyLight(block);
        if ((nextBlock == currentBlock) && (count < 65535)) {
            count++;
        }
        else {
            compressedChunk[packetIndex] = currentBlock;
            compressedChunk[packetIndex + 1] = count >> 8;
            compressedChunk[packetIndex + 2] = count;
            packetIndex += 3;
            currentBlock = nextBlock;
            count = 0;
        }
    }
    compressedChunk[packetIndex] = currentBlock;
    compressedChunk[packetIndex + 1] = count >> 8;
    compressedChunk[packetIndex + 2] = count;
    packetIndex += 3;
    compressedChunk.setPayloadLength(packetIndex);
}

void Compression::decompressChunk(Packet<unsigned char,
    9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE>& compressedChunk,
    Chunk& chunk) {
    // Add blocks
    unsigned int packetIndex = 12;
    unsigned int blockNum = 0;
    while (blockNum < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE) {
        unsigned int count = ((unsigned int)(compressedChunk[packetIndex + 1]) << 8)
            + (unsigned int)(compressedChunk[packetIndex + 2]) + 1 + blockNum;
        while (blockNum < count) {
            chunk.setBlockUnchecked(blockNum, compressedChunk[packetIndex]);
            blockNum++;
        }
        packetIndex += 3;
    }
    // Add skylight
    blockNum = 0;
    while (blockNum < constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE) {
        unsigned int count = ((unsigned int)(compressedChunk[packetIndex + 1]) << 8)
            + (unsigned int)(compressedChunk[packetIndex + 2]) + 1 + blockNum;
        while (blockNum < count) {
            chunk.setSkyLight(blockNum, compressedChunk[packetIndex]);
            blockNum++;
        }
        packetIndex += 3;
    }
}

void Compression::getChunkPosition(Packet<unsigned char,
    9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE>& compressedChunk,
    Position& position) {
    int chunkPosition[3];
    unsigned int packetIndex = 0;
    for (int i = 0; i < 3; i++) {
        chunkPosition[i] = 0;
        for (int shift = 24; shift >= 0; shift -= 8) {
            chunkPosition[i] += compressedChunk[packetIndex] << shift;
            packetIndex++;
        }
    }
    position = chunkPosition;
}