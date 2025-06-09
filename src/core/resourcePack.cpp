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

#include "core/resourcePack.h"

#include "core/log.h"
#include "core/pch.h"

namespace lonelycube {

bool ResourcePack::isTrue(std::basic_istream<char>& stream) const {
    std::string value;
    std::getline(stream, value, '\n');
    value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return !isalpha(c); }),
        value.end());
    std::transform(value.begin(), value.end(), value.begin(),
        [](char c){ return std::tolower(c); });
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
    uint32_t blockID = 0;
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
    for (int blockID = 0; blockID < 255; blockID++) {
        if (m_blockData[blockID].name.size() == 0) {
            continue;
        }
        // Set defaults
        m_blockData[blockID].model = nullptr;
        m_blockData[blockID].blockLight = 0;
        m_blockData[blockID].transparent = false;
        m_blockData[blockID].dimsLight = false;
        m_blockData[blockID].collidable = true;
        m_blockData[blockID].castsAmbientOcclusion = true;

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
            else if (field == "dimsLight") {
                m_blockData[blockID].dimsLight = isTrue(stream);
            }
            else if (field == "castsAmbientOcclusion") {
                m_blockData[blockID].castsAmbientOcclusion = isTrue(stream);
            }
            else if (field == "collidable") {
                m_blockData[blockID].collidable = isTrue(stream);
            }
            else if (field == "model") {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
                std::getline(stream, value, '"');
                // Find the index of the block model
                int modelID = 0;
                while (modelID < m_blockModels.size() && m_blockModels[modelID].name != value) {
                    modelID++;
                }
                if (modelID == m_blockModels.size()) {
                    m_blockModels.push_back(Model());
                    m_blockModels.back().name = value;
                }
                m_blockData[blockID].model = &(m_blockModels[modelID]);
            }
            else if (field == "textureIndices") {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '[');
                std::getline(stream, value, ']');
                std::stringstream line(value);
                while (std::getline(line, value, ',')) {
                    m_blockData[blockID].faceTextureIndices.push_back(std::stoi(value));
                }
            }
            else if (field == "blockLight") {
                std::getline(stream, value, '\n');
                value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return !isdigit(c); }),
                    value.end());
                m_blockData[blockID].blockLight = std::stoi(value);
            }
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
        }
        stream.close();
    }

    // Parse block models
    for (auto& model : m_blockModels) {
        std::ifstream stream(resourcePackPath/"blocks/blockModels"/(model.name + ".json"));
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
                int i = 0;
                while (i < 6 && std::getline(line, value, ',')) {
                    bounds[i] = static_cast<float>(std::stoi(value)) / 16;
                    i++;
                }
                if (i < 6) {
                    continue;
                }
                const int boundIndices[24] = { 0,1,2,3,1,2,3,1,5,0,1,5,0,4,5,3,4,5,3,4,2,0,4,2 };
                for (int i = 0; i < 24; i++) {
                    model.boundingBoxVertices[i] = bounds[boundIndices[i]];
                }
            }
            if (field == "faces") {
                while (true) {  // Runs once per face
                    // Set defaults
                    model.faces.push_back(Face());
                    Face& face = model.faces.back();
                    face.lightingBlock = 6;
                    face.cullFace = -1;
                    face.ambientOcclusion = true;

                    stream.ignore(std::numeric_limits<std::streamsize>::max(), '{');
                    std::string faceString;
                    std::getline(stream, faceString, '}');
                    std::stringstream faceStream(faceString);
                    faceStream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
                    while (!faceStream.eof()) {
                        std::getline(faceStream, field, '"');
                        if (field == "ambientOcclusion") {
                            face.ambientOcclusion = isTrue(faceStream);
                        }
                        if (field == "lighting") {
                            faceStream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
                            std::getline(faceStream, value, '"');
                            if (value == "posY") {
                                face.lightingBlock = 5;
                            }
                            else if (value == "posZ") {
                                face.lightingBlock = 4;
                            }
                            else if (value == "posX") {
                                face.lightingBlock = 3;
                            }
                            else if (value == "negX") {
                                face.lightingBlock = 2;
                            }
                            else if (value == "negZ") {
                                face.lightingBlock = 1;
                            }
                            else if (value == "negY") {
                                face.lightingBlock = 0;
                            }
                            else if (value == "this") {
                                face.lightingBlock = 6;
                            }
                        }
                        if (field == "cullFace") {
                            faceStream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
                            std::getline(faceStream, value, '"');
                            if (value == "negY") {
                                face.cullFace = 0;
                            }
                            else if (value == "negZ") {
                                face.cullFace = 1;
                            }
                            else if (value == "negX") {
                                face.cullFace = 2;
                            }
                            else if (value == "posX") {
                                face.cullFace = 3;
                            }
                            else if (value == "posZ") {
                                face.cullFace = 4;
                            }
                            else if (value == "posY") {
                                face.cullFace = 5;
                            }
                        }
                        if (field == "coordinates") {
                            faceStream.ignore(std::numeric_limits<std::streamsize>::max(), '[');
                            std::getline(faceStream, value, ']');
                            std::stringstream line(value);
                            int i = 0;
                            while (i < 12 && std::getline(line, value, ',')) {
                                face.coords[i] =
                                    static_cast<float>(std::stoi(value)) / 16;
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
                            int i = 0;
                            while (i < 4 && std::getline(line, value, ',')) {
                                face.UVcoords[i] = (float)std::stoi(value) / 16;
                                i++;
                            }
                            if (i < 4) {
                                continue;
                            }
                        }
                    }
                    if (stream.get() != ',') {
                        break;
                    }
                }
            }
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '"');
        }
    }
}

void ResourcePack::getTextureCoordinates(
    float* coords, const float* textureBox, const int textureNum
) {
    coords[0] = 0.0078125f + textureNum % 32 * 0.03125f + textureBox[0] * 0.015625f;
    coords[1] = 0.023475f + textureNum / 32 * 0.03125f - textureBox[1] * 0.015625f;
    coords[2] = coords[0] + 0.015625f - (textureBox[0] + 1.0f - textureBox[2]) * 0.015625f;
    coords[3] = coords[1];
    coords[4] = coords[2];
    coords[5] = coords[1] - 0.015625f + (textureBox[1] + 1.0f - textureBox[3]) * 0.015625f;
    coords[6] = coords[0];
    coords[7] = coords[5];

    // coords[0] = coords[1] = coords[3] = coords[6] = 0.0f;
    // coords[2] = coords[4] = coords[5] = coords[7] = 1.0f;
}

}  // namespace lonelycube
