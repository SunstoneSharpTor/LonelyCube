//#include <GL/glew.h>
#include <glad/glad.h>
#include "stb_image.h"

#include "texture.h"

texture::texture(const std::string& path) : m_rendererID(0), m_filePath(path), m_localBuffer(nullptr), m_width(0), m_height(0), m_BPP(0) {
	//create and bind opengl texture object
	glGenTextures(1, &m_rendererID);
	glBindTexture(GL_TEXTURE_2D, m_rendererID);
	//make the texture flip vertically because textures in opengl are stored starting bottom left
	stbi_set_flip_vertically_on_load(1);
	//load the texture to main memory
	m_localBuffer = stbi_load(path.c_str(), &m_width, &m_height, &m_BPP, 4);
	//set the opengl texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//send the texture to the gpu
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_localBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);
	//if the local buffer contains any data, free it
	if (m_localBuffer) {
		stbi_image_free(m_localBuffer);
	}
}

texture::~texture() {
	glDeleteTextures(1, &m_rendererID);
}

void texture::bind(unsigned int slot /* = 0 */ ) const {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_rendererID);
}

void texture::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}