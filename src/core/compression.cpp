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

#include "compression.h"

#include "core/pch.h"

#include "core/chunk.h"
#include "core/packet.h"

void Compression::compressChunk(Packet<unsigned char,
    6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE>& compressedChunk,
    Chunk& chunk) {
    //  Add blocks
    unsigned char currentBlock = chunk.getBlock(0);
    unsigned char count = 0;
    unsigned int packetIndex = 0;
    unsigned char nextBlock;
    for (unsigned int block = 1; block < constants::CHUNK_SIZE
                                       * constants::CHUNK_SIZE
                                       * constants::CHUNK_SIZE; block++) {
        nextBlock = chunk.getBlock(block);
        if ((nextBlock != currentBlock) && (count < 127)) {
            count++;
        }
        else {
            compressedChunk[packetIndex] = currentBlock;
            compressedChunk[packetIndex + 1] = count;
            packetIndex += 2;
            currentBlock = nextBlock;
            count = 0;
        }
    }
    if (count == 0) {
        compressedChunk[packetIndex] = currentBlock;
        compressedChunk[packetIndex + 1] = count;
        packetIndex += 2;
    }
    //  Add skylight
    currentBlock = chunk.getSkyLight(0);
    count = 0;
    for (unsigned int block = 1; block < constants::CHUNK_SIZE
                                       * constants::CHUNK_SIZE
                                       * constants::CHUNK_SIZE; block++) {
        nextBlock = chunk.getSkyLight(block);
        if ((nextBlock != currentBlock) && (count < 127)) {
            count++;
        }
        else {
            compressedChunk[packetIndex] = currentBlock;
            compressedChunk[packetIndex + 1] = count;
            packetIndex += 2;
            currentBlock = nextBlock;
            count = 0;
        }
    }
    if (count == 0) {
        compressedChunk[packetIndex] = currentBlock;
        compressedChunk[packetIndex + 1] = count;
        packetIndex += 2;
    }
    std::cout << packetIndex << std::endl;
}