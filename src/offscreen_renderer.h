#pragma once
#include <memory>
#include <string>

#include "compute_shader.h"
#include "texture.h"

using std::shared_ptr;
using std::make_shared;

class OffscreenRenderer
{
public:
    OffscreenRenderer();
    
    void execute();

private:

    // Configuration
    // Mode
    bool enableSuperResolution = false;
    bool enableInterpolation = true;
    int generatedFramesCount = 1;
    // Inputs
    float upsampleScale = 1.0f;
    int renderWidth = 1920;
    int renderHeight = 1080;
    int startInputFrame = 10;
    int endInputFrame = 150;
    // LR Inputs (super resolution inputs)
    const std::string inputLrDepthDirectory = "path/to/lr/depth/";
    const std::string inputLrMotionVectorXDirectory = "path/to/lr/motion_vectors_x/";
    const std::string inputLrMotionVectorYDirectory = "path/to/lr/motion_vectors_y/";
    // HR Inputs (interpolation inputs and super resolution color input)
    const std::string inputHrColorDirectory = "path/to/hr/color/";
    const std::string inputHrDepthDirectory = "path/to/hr/depth/";
    const std::string inputHrMotionVectorXDirectory = "path/to/hr/motion_vectors_x/";
    const std::string inputHrMotionVectorYDirectory = "path/to/hr/motion_vectors_y/";
    // Outputs
    const std::string outputDirectory = "path/to/outputs/";
    // Resources (located in MobFGSR/resources/)
    const std::string resourcesDirectory = "path/to/MobFGSR/resources/";
    // Parameters
    float depthDiffThresholdSR = 0.01f;
    float colorDiffThresholdFG = 0.01f;
    float depthDiffThresholdFG = 0.004f;
    float depthScale = 1.0f;
    float depthBias = 0.0f;

    
    struct vec4
    {
        float x;
        float y;
        float z;
        float w;
    };

    struct vec2
    {
        float x;
        float y;

        vec2(): x(0), y(0) {}
        vec2(float x, float y): x(x), y(y) {}
    };

    struct alignas(16) UniformBlock
    {
        vec4 render_size;
        vec4 presentation_size;
        vec4 delta;
        vec2 jitter_offset;
        float depth_diff_threshold_sr;
        float color_diff_threshold_fg;
        float depth_diff_threshold_fg;
        float depth_scale;
        float depth_bias;
        float render_scale;
    };
    
    const int localSize = 8;

    // Uniform buffer
    unsigned int uniformBuffer;
    
    int presentationWidth;
    int presentationHeight;
    
    int groupX_LR;
    int groupY_LR;
    int groupZ_LR;
    int groupX_HR;
    int groupY_HR;
    int groupZ_HR;
    
    int currentInputFrame;
    int currentOutputFrame;
    float delta;
    bool isFirstCycleCompleted;
    bool isRenderedFrame;
    bool isGeneratedFrame;

    static constexpr int jitterSequenceLength = 12;
    int jitterOffsetIndex;
    vec2 jitterOffset;
    const vec2 jitterSequence[jitterSequenceLength] =
    {
        vec2(-0.25f, -0.25f),
        vec2(+0.25f, -0.25f),
        vec2(-0.25f, +0.25f),
        vec2(+0.25f, -0.25f),
        vec2(-0.25f, -0.25f),
        vec2(+0.25f, +0.25f),
        vec2(-0.25f, -0.25f),
        vec2(+0.25f, -0.25f),
        vec2(-0.25f, +0.25f),
        vec2(+0.25f, -0.25f),
        vec2(-0.25f, -0.25f),
        vec2(+0.25f, +0.25f),
    };

    // Compute shaders
    shared_ptr<ComputeShader> loadDepthCS;
    shared_ptr<ComputeShader> loadMotionVectorCS;
    shared_ptr<ComputeShader> loadLRColorCS;
    shared_ptr<ComputeShader> dilateCS;
    shared_ptr<ComputeShader> clearCS;
    shared_ptr<ComputeShader> reprojectCS_I;
    shared_ptr<ComputeShader> fillCS;
    shared_ptr<ComputeShader> warpCS_I;
    shared_ptr<ComputeShader> upsampleFirstFrameCS;
    shared_ptr<ComputeShader> blendHistoryCS;

    // Raw inputs
    shared_ptr<Texture> rawInputHRColor;
    shared_ptr<Texture> rawInputDepth;
    shared_ptr<Texture> rawInputMotionVectorX;
    shared_ptr<Texture> rawInputMotionVectorY;
    
    // Inputs
    shared_ptr<Texture> inputColor;
    shared_ptr<Texture> inputDepth;
    shared_ptr<Texture> inputMotionVector;

    // LUTs
    shared_ptr<Texture> sampleLut;

    // Current
    shared_ptr<Texture> currentDilatedDepth;
    shared_ptr<Texture> currentDilatedMotionVector;
    shared_ptr<Texture> currentHRColor;

    // Previous
    shared_ptr<Texture> previousDilatedDepth;
    shared_ptr<Texture> previousDilatedMotionVector;
    shared_ptr<Texture> previousHRColor;

    // Frame generation
    shared_ptr<Texture> reprojection;
    shared_ptr<Texture> filledReprojection;
    shared_ptr<Texture> frameGenerationResult;

    // Output
    shared_ptr<Texture> outputColor;
    
    void load();
    void render();
    void save();

    void bindUniformBuffer();
    void swapBuffers();
    void processInputs();
    void preprocess();
    void interpolate();
    void upsampleFirstFrame();
    void superSample();
};
