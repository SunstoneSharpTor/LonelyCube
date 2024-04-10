#pragma once

#include <string>
#include <unordered_map>

#include "glm/glm.hpp"

using namespace std;

struct shaderProgramSources {
	string vertexSource;
	string fragmentSource;
};

class shader {
private:
	string m_vertexFilePath, m_fragmentFilePath;
	unsigned int m_rendererID;
	unordered_map<string, int> m_uniformLocationCache;
public:
	shader(const string& vertexFilePath, const string& fragmantFilePath);
	~shader();

	void bind() const;
	void unbind() const;

	//set uniforms
	void setUniform1i(const string& name, int value);
	void setUniform1f(const string& name, float value);
	void setUniform4f(const string& name, float v0, float v1, float v2, float v3);
	void setUniformMat4f(const string& name, const glm::mat4& value);
private:
	shaderProgramSources parseShaders(const string& vertexFilePath, const string& fragmentFilePath);
	unsigned int compileShader(unsigned int type, const string& source);
	unsigned int createShader(const string& vertexShader, const string& fragmentShader);

	int getUniformLocation(const string& name);
};