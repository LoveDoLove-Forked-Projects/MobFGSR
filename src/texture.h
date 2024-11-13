#pragma once
#include <string>
#include <glad/glad.h>

class Texture
{
public:
	Texture(GLenum format, GLsizei width, GLsizei height, GLint filter);

	~Texture();

	void loadFromFile(const std::string& path, GLenum sourceFormat, GLenum sourceType, bool flipVertical = false);

	void loadLUT(const std::string& path);

	void bindImageUnit(GLuint imageUnit, GLenum access) const;

	void bindTexture(GLenum textureUnit) const;

	unsigned int getID() const { return textureID; }

	void saveAsPNG(const char* path, int sourcePixelSize = 4) const;

	void saveLUT(const char* path) const;
private:
	unsigned int textureID;
	GLenum internalFormat;
	int width;
	int height;
};

