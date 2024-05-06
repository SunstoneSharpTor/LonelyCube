#pragma once

class indexBuffer {
private:
	unsigned int m_rendererID;
	unsigned int m_count;
public:
	indexBuffer();
	indexBuffer(const unsigned int* data, unsigned int count);
	~indexBuffer();

	void bind() const;
	void unbind() const;

	inline unsigned int getCount() const {
		return m_count;
	}
};