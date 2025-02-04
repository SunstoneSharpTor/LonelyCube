/*
  Lonely Cube, a voxel game
  Copyright (C) g 2024-2025 Bertie Cartwright

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

#include "core/pch.h"

#include "glm/glm.hpp"

namespace lonelycube::client {

struct shaderProgramSources {
    std::string vertexSource;
    std::string fragmentSource;
};

class Shader {
private:
    std::string m_vertexFilePath, m_fragmentFilePath;
    uint32_t m_rendererID;
    std::unordered_map<std::string, int> m_uniformLocationCache;
public:
    Shader(const std::string& vertexFilePath, const std::string& fragmantFilePath);
    ~Shader();

    void bind() const;
    void unbind() const;

    //set uniforms
    void setUniform1i(const std::string& name, int value);
    void setUniform1f(const std::string& name, float value);
    void setUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
    void setUniformMat4f(const std::string& name, const glm::mat4& value);
private:
    shaderProgramSources parseShaders(const std::string& vertexFilePath, const std::string& fragmentFilePath);
    uint32_t compileShader(uint32_t type, const std::string& source);
    uint32_t createShader(const std::string& vertexShader, const std::string& fragmentShader);

    int getUniformLocation(const std::string& name);
};

}  // namespace lonelycube::client
