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

#include "client/computeShader.h"

#include "core/pch.h"

#include "client/renderer.h"

namespace client {

ComputeShader::ComputeShader(const std::string& filePath)
	: m_filePath(filePath), m_rendererID(0) {
    std::string shaderSource = parseShader(filePath);
    m_rendererID = createShader(shaderSource);
}

ComputeShader::~ComputeShader() {
    glDeleteProgram(m_rendererID);
}

std::string ComputeShader::parseShader(const std::string& filePath) {
    std::stringstream source;
    std::string line;
    std::ifstream stream(filePath);
    #ifdef GLES3
        source << "#version 310 es\n";
    #else
        source << "#version 450 core\n";
    #endif
    while (getline(stream, line)) {
        source << line << "\n";
    }
    stream.close();
    return source.str();
}

unsigned int ComputeShader::compileShader(const std::string& source) {
    unsigned int id = glCreateShader(GL_COMPUTE_SHADER);
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
        std::cout << "Failed to compile shader. " << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

unsigned int ComputeShader::createShader(const std::string& shader) {
    unsigned int program = glCreateProgram();
    unsigned int cs = compileShader(shader);

    glAttachShader(program, cs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(cs);

    return program;
}

void ComputeShader::bind() const {
    glUseProgram(m_rendererID);
}

void ComputeShader::unbind() const {
    glUseProgram(0);
}

void ComputeShader::setUniform1i(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void ComputeShader::setUniform1f(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void ComputeShader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3) {
    glUniform4f(getUniformLocation(name), v0, v1, v2, v3);
}

void ComputeShader::setUniformMat4f(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

int ComputeShader::getUniformLocation(const std::string& name) {
    //if the location has already been cached, return the location from the hash table
    if (m_uniformLocationCache.find(name) != m_uniformLocationCache.end()) {
        return m_uniformLocationCache[name];
    }
    //otherwise, get the location, and cache it
    int location = glGetUniformLocation(m_rendererID, name.c_str());
    if (location == -1) {
        std::cout << "Warning: uniform " << name << " doesn't exist." << std::endl;
    }
    //cache the uniform location
    m_uniformLocationCache[name] = location;
    return location;
}

}  // namespace client