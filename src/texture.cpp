#include "texture.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"


Texture::Texture(GLenum format, GLsizei w, GLsizei h, GLint filter) :
	internalFormat(format), width(w), height(h)
{
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, w, h);
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
	glDeleteTextures(0, &textureID);
}

void Texture::loadFromFile(const std::string& path, GLenum sourceFormat, GLenum sourceType, bool flipVertical)
{
	if (flipVertical)
	{
		stbi_set_flip_vertically_on_load(true);
	}
	int w, h, c;
	unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
	if (data)
	{
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, sourceFormat, sourceType, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::cout << "Failed to load texture " << path << std::endl;
	}
	stbi_image_free(data);
}

void Texture::loadLUT(const std::string& path)
{
	int width, height;
	float* data;
	const char* err;
	int ret = LoadEXR(&data, &width, &height, path.c_str(), &err);
	if (ret < 0)
	{
		std::cerr << err << "\n";
		return;
	}

	// RGBA -> A
	float* resizedData = new float[width * height];
	for (int i = 0; i < width * height; i++)
	{
		resizedData[i] = data[i * 4 + 3];
	}
	free(data);
	
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, static_cast<void*>(resizedData));
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] resizedData;
}

void Texture::bindImageUnit(GLuint imageUnit, GLenum access) const
{
	glBindImageTexture(imageUnit, textureID, 0, GL_FALSE, 0, access, internalFormat);
}

void Texture::bindTexture(GLenum textureUnit) const
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::saveAsPNG(const char* path, int sourcePixelSize) const
{
	GLubyte* data = new GLubyte[width * height * sourcePixelSize];
	glBindTexture(GL_TEXTURE_2D, textureID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	stbi_write_png(path, width, height, 4, data, 0);
	delete[] data;
}

void Texture::saveLUT(const char* path) const
{
	float* data = new float[width * height];
	glBindTexture(GL_TEXTURE_2D, textureID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, data);

	const char* err;
	SaveEXR(data, width, height, 1, 0, path, &err);

	delete[] data;
}
