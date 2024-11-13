#pragma once
#include <string>

class ComputeShader
{
public:
    ComputeShader(const std::string& shaderPath);

    unsigned int getID() const { return shaderID; }
    void use() const;
    void dispatch(int numGroupX, int numGroupY, int numGroupZ) const;
private:
    unsigned int shaderID;
};
