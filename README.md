# Mob-FGSR
Official code for SIGGRAPH 2024 paper "Mob-FGSR: Frame Generation and Super Resolution for Mobile Real-Time Rendering".  
| [Project Page](https://mob-fgsr.github.io/) | [Paper](https://mob-fgsr.github.io/resources/paper/SIGGRAPH_Conf_Mob_FGSR.pdf) |  
## Prerequisites
- C++ Compiler - needs to support at least C++11
- CMake
- OpenGL 4.3 or higher
## Building Instructions
```
git clone https://github.com/Mob-FGSR/MobFGSR.git
cd MobFGSR
mkdir build
cd build
cmake ..
```
## Configuration
Before running, edit offscreen_renderer.h (located in MobFGSR/src/) to configure super-sampling mode and IO settings. This is a guide for how to set these fields:  
``` C++
// Enable or disable super resolution
bool enableSuperResolution = false;       
// Enable or disable interpolation (can be enabled with super resolution)  
bool enableInterpolation = true;            
// How many frames will be generated between two rendered frames (ignored when interpolation is disabled)
int generatedFramesCount = 1;               
// If super resolution is enabled, upsampleScale should be 2.0f, otherwise upsampleScale will be ignored
float upsampleScale = 1.0f;
// Width of image inputs
int renderWidth = 1920;
// Height of image inputs
int renderHeight = 1080;
// First frame of inputs (the image file name should be like "0010.png")
int startInputFrame = 10;
// Last frame of inputs (the image file name should be like "0150.png")
int endInputFrame = 150;
// Absolute directories paths of low resolution inputs for super resolution pipeline (Depth and motion vectors are packed before input. Please refer to resources/IO/LoadDepth.comp and resources/IO/LoadMotionVector.comp)
const std::string inputLrDepthDirectory = "path/to/lr/depth/";
const std::string inputLrMotionVectorXDirectory = "path/to/lr/motion_vectors_x/";
const std::string inputLrMotionVectorYDirectory = "path/to/lr/motion_vectors_y/";
// Absolute directories paths of high resolution inputs for interpolation pipeline (However, we still use high resolution color inputs super resolution pipeline to make them "jittered". Please refer to resources/IO/LoadLRColor.comp for details)
const std::string inputHrColorDirectory = "path/to/hr/color/";
const std::string inputHrDepthDirectory = "path/to/hr/depth/";
const std::string inputHrMotionVectorXDirectory = "path/to/hr/motion_vectors_x/";
const std::string inputHrMotionVectorYDirectory = "path/to/hr/motion_vectors_y/";
// Absolute path for outputs directory
const std::string outputDirectory = "path/to/outputs/";
// Absolute path for resources (located in MobFGSR/resources/)
const std::string resourcesDirectory = "path/to/MobFGSR/resources/";
// Parameters for compute shaders
float depthDiffThresholdSR = 0.01f;
float colorDiffThresholdFG = 0.01f;
float depthDiffThresholdFG = 0.004f;
float depthScale = 1.0f;
float depthBias = 0.0f;
```
## Third Party
- [GLFW](https://www.glfw.org/)
- [GLAD](https://glad.dav1d.de/)
- [stb_image](https://github.com/nothings/stb/tree/master)
- [tinyexr](https://github.com/syoyo/tinyexr)