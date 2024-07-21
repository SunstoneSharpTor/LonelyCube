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

template<typename T, int maxPayloadLength>
class Packet {
private:
    short m_packetType;
    short m_payloadLength;
    int m_peerID;
    std::array<T, maxPayloadLength> m_payload;
public:
    Packet(int peerID, short packetType, short payloadLength) :
    m_packetType(packetType), m_payloadLength(payloadLength), m_peerID(peerID) { }

    Packet() : m_payloadLength(0) {};

    short getPeerID() {
        return m_peerID;
    }

    short getPacketType() {
        return m_packetType;
    }

    short getPayloadLength() {
        return m_payloadLength;
    }

    T operator[](int index) const {
        return m_payload[index];
    }

    T& operator[](int index) {
        return m_payload[index];
    } 

    short getSize() {
        return sizeof(int) + 2 * sizeof(short) + m_payloadLength * sizeof(T);
    }

    T* getPayloadAddress(int index) {
        return &(m_payload[0]);
    }
};