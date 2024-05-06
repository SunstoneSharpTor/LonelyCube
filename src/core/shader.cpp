#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "shader.h"
#include "renderer.h"

shader::shader(const std::string& vertexFilePath, const std::string& fragmentFilePath)
	: m_vertexFilePath(vertexFilePath), m_fragmentFilePath(fragmentFilePath), m_rendererID(0) {
    shaderProgramSources shaderSources = parseShaders(vertexFilePath, fragmentFilePath);
    m_rendererID = createShader(shaderSources.vertexSource, shaderSources.fragmentSource);
}

shader::~shader() {
    glDeleteProgram(m_rendererID);
}

shaderProgramSources shader::parseShaders(const std::string& vertexFilePath, const std::string& fragmentFilePath) {
    std::stringstream sources[2];
    std::string line;
    std::ifstream vertexStream(vertexFilePath);
    std::ifstream fragmentStream(fragmentFilePath);
    std::ifstream* stream = &vertexStream;
    for (int shaderNum = 0; shaderNum < 2; shaderNum++) {
        while (getline(*stream, line)) {
            sources[shaderNum] << line << "\n";
        }
        stream->close();
        stream = &fragmentStream;
    }
    return { sources[0].str(), sources[1].str() };
}

unsigned int shader::compileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
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

unsigned int shader::createShader(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void shader::bind() const {
    glUseProgram(m_rendererID);
}

void shader::unbind() const {
    glUseProgram(0);
}

void shader::setUniform1i(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void shader::setUniform1f(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void shader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3) {
    glUniform4f(getUniformLocation(name), v0, v1, v2, v3);
}

void shader::setUniformMat4f(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

int shader::getUniformLocation(const std::string& name) {
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