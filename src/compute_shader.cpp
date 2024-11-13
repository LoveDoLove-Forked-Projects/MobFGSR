#include "compute_shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <glad/glad.h>

ComputeShader::ComputeShader(const std::string& shaderPath)
{
	std::string shaderCode;
	std::ifstream shaderFile;

	shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		shaderFile.open(shaderPath);
		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();
		shaderFile.close();
		shaderCode = shaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR: Cannot read compute shader from file " << shaderPath << std::endl;
	}
	const char* source = shaderCode.c_str();
	
	int success;
	char infoLog[1024];

	unsigned int computeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(computeShader, 1, &source, NULL);
	glCompileShader(computeShader);
	glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(computeShader, 1024, NULL, infoLog);
		std::cout << "ERROR: Compute shader compilation failed in " << shaderPath << "\n" << infoLog << "\n";
	}

	shaderID = glCreateProgram();
	glAttachShader(shaderID, computeShader);
	glLinkProgram(shaderID);
	glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderID, 1024, NULL, infoLog);
		std::cout << "ERROR: Compute shader linking failed in " << shaderPath << "\n" << infoLog << "\n";
	}
	glDeleteShader(computeShader);
}

void ComputeShader::use() const
{
	glUseProgram(shaderID);
}

void ComputeShader::dispatch(int numGroupX, int numGroupY, int numGroupZ) const
{
	glDispatchCompute(numGroupX, numGroupY, numGroupZ);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
