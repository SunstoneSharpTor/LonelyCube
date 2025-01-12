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

namespace client {

class ComputeShader {
private:
	std::string m_filePath;
	uint32_t m_rendererID;
	std::unordered_map<std::string, int> m_uniformLocationCache;
public:
	ComputeShader(const std::string& filePath);
	~ComputeShader();

	void bind() const;
	void unbind() const;

	//set uniforms
	void setUniform1i(const std::string& name, int value);
	void setUniform1f(const std::string& name, float value);
	void setUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void setUniformMat4f(const std::string& name, const glm::mat4& value);
	void setUniformVec3(const std::string& name, const glm::vec3& value);
private:
	std::string parseShader(const std::string& filePath);
	uint32_t compileShader(const std::string& source);
	uint32_t createShader(const std::string& shader);

	int getUniformLocation(const std::string& name);
};

}  // namespace client
