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

#include "client/graphics/shader.h"

#include "core/pch.h"

#include "client/graphics/renderer.h"
#include "core/log.h"

namespace lonelycube::client {

Shader::Shader(const std::string& vertexFilePath, const std::string& fragmentFilePath)
        : m_vertexFilePath(vertexFilePath), m_fragmentFilePath(fragmentFilePath), m_rendererID(0) {
    shaderProgramSources shaderSources = parseShaders(vertexFilePath, fragmentFilePath);
    m_rendererID = createShader(shaderSources.vertexSource, shaderSources.fragmentSource);
}

Shader::~Shader() {
    #ifndef GLES3
    glDeleteProgram(m_rendererID);
    #endif
}

shaderProgramSources Shader::parseShaders(const std::string& vertexFilePath, const std::string& fragmentFilePath) {
    std::stringstream sources[2];
    std::string line;
    std::ifstream vertexStream(vertexFilePath);
    std::ifstream fragmentStream(fragmentFilePath);
    std::ifstream* stream = &vertexStream;
    for (int shaderNum = 0; shaderNum < 2; shaderNum++) {
        #ifdef GLES3
            sources[shaderNum] << "#version 310 es\n";
        #else
            sources[shaderNum] << "#version 450 core\n";
        #endif
        while (getline(*stream, line)) {
            sources[shaderNum] << line << "\n";
        }
        stream->close();
        stream = &fragmentStream;
    }
    return { sources[0].str(), sources[1].str() };
}

uint32_t Shader::compileShader(uint32_t type, const std::string& source) {
    uint32_t id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        LOG(std::string("Failed to compile shader. ") + message);
        glDeleteShader(id);
        return 0;
    }

    return id;
}

uint32_t Shader::createShader(const std::string& vertexShader, const std::string& fragmentShader) {
    uint32_t program = glCreateProgram();
    uint32_t vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void Shader::bind() const {
    glUseProgram(m_rendererID);
}

void Shader::unbind() const {
    glUseProgram(0);
}

void Shader::setUniform1i(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setUniform1f(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3) {
    glUniform4f(getUniformLocation(name), v0, v1, v2, v3);
}

void Shader::setUniformMat4f(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

int Shader::getUniformLocation(const std::string& name) {
    //if the location has already been cached, return the location from the hash table
    if (m_uniformLocationCache.find(name) != m_uniformLocationCache.end()) {
        return m_uniformLocationCache[name];
    }
    //otherwise, get the location, and cache it
    int location = glGetUniformLocation(m_rendererID, name.c_str());
    if (location == -1) {
        LOG("Warning: uniform " + name + " doesn't exist.");
    }
    //cache the uniform location
    m_uniformLocationCache[name] = location;
    return location;
}

}  // namespace lonelycube::client
