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

    Packet() {};

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