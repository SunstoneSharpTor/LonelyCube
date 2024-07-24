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

#pragma once

#include "enet/enet.h"

#include "pch.h"

enum PacketType {
    ClientConnection
};

template<typename T, unsigned int maxPayloadLength>
class Packet {
private:
    unsigned short m_packetType;
    unsigned short m_peerID;
    unsigned int m_payloadLength;
    std::array<T, maxPayloadLength> m_payload;
public:
    Packet(int peerID, short packetType, short payloadLength) :
    m_packetType(packetType), m_peerID(peerID), m_payloadLength(payloadLength) { }

    Packet() : m_payloadLength(0) {};

    unsigned short getPeerID() {
        return m_peerID;
    }

    unsigned short getPacketType() {
        return m_packetType;
    }

    unsigned int getPayloadLength() {
        return m_payloadLength;
    }

    T operator[](unsigned int index) const {
        return m_payload[index];
    }

    T& operator[](unsigned int index) {
        return m_payload[index];
    } 

    unsigned int getSize() {
        constexpr unsigned int baseSize = sizeof(Packet<T, 1>) - sizeof(T);
        return baseSize + m_payloadLength * sizeof(T);
    }

    T* getPayloadAddress(unsigned int index) {
        return &(m_payload[0]);
    }
};