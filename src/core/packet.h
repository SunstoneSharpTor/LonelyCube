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

#pragma once

#include "pch.h"

namespace lonelycube {

enum PacketType {
    ClientConnection, ChunkSent, ClientPosition, BlockReplaced, ChunkRequest
};

template<typename T, uint32_t maxPayloadLength>
class Packet {
private:
    uint16_t m_packetType;
    uint16_t m_peerID;
    uint32_t m_payloadLength;
    std::array<T, maxPayloadLength> m_payload;
public:
    Packet(int peerID, int16_t packetType, int16_t payloadLength) :
    m_packetType(packetType), m_peerID(peerID), m_payloadLength(payloadLength) { }

    Packet() : m_payloadLength(0) {};

    uint16_t getPeerID() const {
        return m_peerID;
    }

    void setPeerID(const uint16_t peerID) {
        m_peerID = peerID;
    }

    uint16_t getPacketType() const {
        return m_packetType;
    }

    uint32_t getPayloadLength() const {
        return m_payloadLength;
    }

    void setPayloadLength(const uint32_t payloadLength) {
        m_payloadLength = payloadLength;
    }

    T operator[](const uint32_t index) const {
        return m_payload[index];
    }

    T& operator[](const uint32_t index) {
        return m_payload[index];
    } 

    uint32_t getSize() const {
        constexpr uint32_t baseSize = sizeof(Packet<T, 1>) - sizeof(T);
        return baseSize + m_payloadLength * sizeof(T);
    }

    const T* getPayloadAddress() const {
        return m_payload.data();
    }
};

}  // namespace lonelycube
