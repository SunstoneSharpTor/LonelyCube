#pragma once

#include <string>
#include <unordered_map>

#include "glm/glm.hpp"

struct shaderProgramSources {
	std::string vertexSource;
	std::string fragmentSource;
};

class shader {
private:
	std::string m_vertexFilePath, m_fragmentFilePath;
	unsigned int m_rendererID;
	std::unordered_map<std::string, int> m_uniformLocationCache;
public:
	shader(const std::string& vertexFilePath, const std::string& fragmantFilePath);
	~shader();

	void bind() const;
	void unbind() const;

	//set uniforms
	void setUniform1i(const std::string& name, int value);
	void setUniform1f(const std::string& name, float value);
	void setUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void setUniformMat4f(const std::string& name, const glm::mat4& value);
private:
	shaderProgramSources parseShaders(const std::string& vertexFilePath, const std::string& fragmentFilePath);
	unsigned int compileShader(unsigned int type, const std::string& source);
	unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader);

	int getUniformLocation(const std::string& name);
};