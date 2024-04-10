#pragma once

#include "renderer.h"

using namespace std;

class texture {
private:
	unsigned int m_rendererID;
	string m_filePath;
	unsigned char* m_localBuffer;
	int m_width, m_height, m_BPP;
public:
	texture(const::string& path);
	~texture();

	void bind(unsigned int slot = 0) const;
	void unbind() const;

	inline int getWidth() const { return m_width; }
	inline int getHeight() const { return m_height; }
};