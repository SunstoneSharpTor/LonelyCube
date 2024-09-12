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

#include "core/resourcePack.h"

#include "core/pch.h"

bool ResourcePack::isTrue(std::basic_istream<char>& stream) const {
    std::string value;
    std::getline(stream, value, '\n');
    value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return !isalpha(c); }),
        value.end());
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c){ return std::tolower(c); });
    if (value == "true") {
        return true;
    }
    return false;
}

ResourcePack::ResourcePack(std::filesystem::path resourcePackPath) {
    // Parse block names
    std::ifstream stream(resourcePackPath/"blocks/blockNames.json");
    stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
    std::string name;
    unsigned int blockID = 0;
    while (!stream.eof()) {
        std::getline(stream, name, '"');
        m_blockData[blockID].name = name;
        blockID++;
        if (blockID > 255) {
            break;
        }
        stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
    }
    stream.close();

    // Parse block data
    for (unsigned char blockID = 0; blockID < 255; blockID++) {
        if (m_blockData[blockID].name.size() == 0) {
            continue;
        }
        // Set defaults
        m_blockData[blockID].modelID = 0;
        bool transparent = false;
        bool dimsLight = false;
        bool collidable = true;
        for (int i = 0; i < maxNumFaces; i++) {
            m_blockData[blockID].faceTextureIndices[i] = 0;
        }

        std::ifstream stream(resourcePackPath/"blocks/blockData"/(m_blockData[blockID].name +
            ".json"));
        if (!stream.is_open()) {
            continue;
        }
        stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
        std::string field, value;
        while (!stream.eof()) {
            std::getline(stream, field, '"');
            if (field == "transparrent") {
                m_blockData[blockID].transparent = isTrue(stream);
            }
            if (field == "dimsLight") {
                m_blockData[blockID].dimsLight = isTrue(stream);
            }
            if (field == "collidable") {
                m_blockData[blockID].collidable = isTrue(stream);
            }
            if (field == "model") {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
                std::getline(stream, value, '"');
                // Find the index of the block model
                unsigned char modelID = 0;
                while (modelID < 255 && m_blockModels[modelID].name != value) {
                    if (m_blockModels[modelID].name.length() == 0) {
                        m_blockModels[modelID].name = value;
                        break;
                    }
                    modelID++;
                }
                m_blockData[blockID].modelID = modelID;
            }
            if (field == "textureIndices") {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '[');
                std::getline(stream, value, ']');
                std::stringstream line(value);
                unsigned char i = 0;
                while (i < maxNumFaces && std::getline(line, value, ',')) {
                    m_blockData[blockID].faceTextureIndices[i] = std::stoi(value);
                    i++;
                }
            }
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
        }
        stream.close();
    }

    // Parse block models
    for (unsigned char modelID = 0; modelID < 255; modelID++) {
        if (m_blockModels[modelID].name.size() == 0) {
            continue;
        }
        // Set defaults
        m_blockModels[modelID].numFaces = 0;

        std::ifstream stream(resourcePackPath/"blocks/blockModels"/(m_blockModels[modelID].name +
            ".json"));
        if (!stream.is_open()) {
            continue;
        }
        stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
        std::string field, value;
        while (!stream.eof()) {
            std::getline(stream, field, '"');
            if (field == "boundingBox") {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '[');
                std::getline(stream, value, ']');
                std::stringstream line(value);
                float bounds[6];
                unsigned char i = 0;
                while (i < 6 && std::getline(line, value, ',')) {
                    // Slightly scale up the point to prevent z-fighting with the block
                    bounds[i] = ((float)std::stoi(value) / 16 - 0.5f) * 1.002f + 0.5f;
                    i++;
                }
                if (i < 6) {
                    continue;
                }
                unsigned char boundIndices[24] = {0,1,2,3,1,2,3,1,5,0,1,5,0,4,5,3,4,5,3,4,2,0,4,2};
                for (unsigned char i = 0; i < 24; i++) {
                    m_blockModels[modelID].boundingBoxVertices[i] = bounds[boundIndices[i]];
                }
            }
            if (field == "faces") {
                unsigned char faceNum = 0;
                while (true) {  // Runs once per face
                    // Set defaults
                    m_blockModels[modelID].faces[faceNum].cullFace = -1;
                    m_blockModels[modelID].faces[faceNum].ambientOcclusion = true;

                    stream.ignore(std::numeric_limits<std::streamsize>::max(), '{');
                    std::string face;
                    std::getline(stream, face, '}');
                    std::stringstream faceStream(face);
                    faceStream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
                    while (!faceStream.eof()) {
                        std::getline(faceStream, field, '"');
                        if (field == "ambientOcclusion") {
                            m_blockModels[modelID].faces[faceNum].ambientOcclusion =
                                isTrue(faceStream);
                        }
                        if (field == "cullFace") {
                            faceStream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
                            std::getline(faceStream, value, '"');
                            if (value == "negY") {
                                m_blockModels[modelID].faces[faceNum].cullFace = 0;
                            }
                            else if (value == "negZ") {
                                m_blockModels[modelID].faces[faceNum].cullFace = 1;
                            }
                            else if (value == "negX") {
                                m_blockModels[modelID].faces[faceNum].cullFace = 2;
                            }
                            else if (value == "posX") {
                                m_blockModels[modelID].faces[faceNum].cullFace = 3;
                            }
                            else if (value == "posZ") {
                                m_blockModels[modelID].faces[faceNum].cullFace = 4;
                            }
                            else if (value == "posY") {
                                m_blockModels[modelID].faces[faceNum].cullFace = 5;
                            }
                        }
                        if (field == "coordinates") {
                            faceStream.ignore(std::numeric_limits<std::streamsize>::max(), '[');
                            std::getline(faceStream, value, ']');
                            std::stringstream line(value);
                            unsigned char i = 0;
                            while (i < 12 && std::getline(line, value, ',')) {
                                // Slightly scale up the point to prevent tiny holes in the mesh
                                m_blockModels[modelID].faces[faceNum].coords[i] =
                                    ((float)std::stoi(value) / 16 - 0.5f) * 1.0008f + 0.5f;
                                i++;
                            }
                            if (i < 12) {
                                continue;
                            }
                        }
                        if (field == "uv") {
                            faceStream.ignore(std::numeric_limits<std::streamsize>::max(), '[');
                            std::getline(faceStream, value, ']');
                            std::stringstream line(value);
                            unsigned char i = 0;
                            while (i < 4 && std::getline(line, value, ',')) {
                                m_blockModels[modelID].faces[faceNum].UVcoords[i] = std::stoi(value);
                                i++;
                            }
                            if (i < 4) {
                                continue;
                            }
                        }
                    }
                    faceNum++;
                    if (stream.get() != ',') {
                        break;
                    }
                }
                m_blockModels[modelID].numFaces = faceNum;
            }
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
        }
    }
}