#include "offscreen_renderer.h"

#include "texture.h"
#include <sstream>
#include <iomanip>
#include <utility>

OffscreenRenderer::OffscreenRenderer()
{
    if (!enableInterpolation)
    {
        generatedFramesCount = 0;
    }
    if (!enableSuperResolution)
    {
        upsampleScale = 1.0f;
    }
    
    presentationWidth = static_cast<int>(static_cast<float>(renderWidth) * upsampleScale);
    presentationHeight = static_cast<int>(static_cast<float>(renderHeight) * upsampleScale);

    const int localSize = 8;
    groupX_LR = (renderWidth + localSize - 1) / localSize;
    groupY_LR = (renderHeight + localSize - 1) / localSize;
    groupZ_LR = 1;
    groupX_HR = (presentationWidth + localSize - 1) / localSize;
    groupY_HR = (presentationHeight + localSize - 1) / localSize;
    groupZ_HR = 1;
    
    // Compute shaders
    loadDepthCS                 = make_shared<ComputeShader>(resourcesDirectory + "IO/LoadDepth.comp");
    loadMotionVectorCS          = make_shared<ComputeShader>(resourcesDirectory + "IO/LoadMotionVector.comp");
    loadLRColorCS               = make_shared<ComputeShader>(resourcesDirectory + "IO/LoadLRColor.comp");
    dilateCS                    = make_shared<ComputeShader>(resourcesDirectory + "Preprocessing/Dilate.comp");
    clearCS                     = make_shared<ComputeShader>(resourcesDirectory + "FrameGeneration/Clear.comp");
    reprojectCS_I               = make_shared<ComputeShader>(resourcesDirectory + "FrameGeneration/Reproject_I.comp");
    fillCS                      = make_shared<ComputeShader>(resourcesDirectory + "FrameGeneration/Fill.comp");
    warpCS_I                    = make_shared<ComputeShader>(resourcesDirectory + "FrameGeneration/Warp_I.comp");
    upsampleFirstFrameCS        = make_shared<ComputeShader>(resourcesDirectory + "SuperResolution/UpsampleFirstFrame.comp");
    blendHistoryCS              = make_shared<ComputeShader>(resourcesDirectory + "SuperResolution/BlendHistory.comp");

    
    // Textures
    if (enableSuperResolution)
    {
        rawInputHRColor         = make_shared<Texture>(GL_RGBA8, presentationWidth, presentationHeight, GL_NEAREST);
    }
    else
    {
        rawInputHRColor         = nullptr;
    }
    rawInputDepth               = make_shared<Texture>(GL_RGBA8, renderWidth, renderHeight, GL_NEAREST);
    rawInputMotionVectorX       = make_shared<Texture>(GL_RGBA8, renderWidth, renderHeight, GL_NEAREST);
    rawInputMotionVectorY       = make_shared<Texture>(GL_RGBA8, renderWidth, renderHeight, GL_NEAREST);

    inputColor                  = make_shared<Texture>(GL_RGBA8, renderWidth, renderHeight, GL_LINEAR);
    inputDepth                  = make_shared<Texture>(GL_R32F, renderWidth, renderHeight, GL_NEAREST);
    inputMotionVector           = make_shared<Texture>(GL_RG16F, renderWidth, renderHeight, GL_NEAREST);

    sampleLut                   = make_shared<Texture>(GL_R16F, 128, 128, GL_LINEAR);

    currentDilatedDepth         = make_shared<Texture>(GL_R32F, renderWidth, renderHeight, GL_NEAREST);
    currentDilatedMotionVector  = make_shared<Texture>(GL_RG16F, renderWidth, renderHeight, GL_NEAREST);
    
    if (enableInterpolation)
    {
        currentHRColor              = make_shared<Texture>(GL_RGBA8, presentationWidth, presentationHeight, GL_LINEAR);
        previousDilatedDepth        = make_shared<Texture>(GL_R32F, renderWidth, renderHeight, GL_NEAREST);
        previousDilatedMotionVector = make_shared<Texture>(GL_RG16F, renderWidth, renderHeight, GL_NEAREST);
        previousHRColor             = make_shared<Texture>(GL_RGBA8, presentationWidth, presentationHeight, GL_LINEAR);
    }
    else if (enableSuperResolution)
    {
        currentHRColor              = make_shared<Texture>(GL_RGBA8, presentationWidth, presentationHeight, GL_LINEAR);
        previousDilatedDepth        = make_shared<Texture>(GL_R32F, renderWidth, renderHeight, GL_NEAREST);
        previousDilatedMotionVector = nullptr;
        previousHRColor             = make_shared<Texture>(GL_RGBA8, presentationWidth, presentationHeight, GL_LINEAR);
    }
    
    reprojection                = make_shared<Texture>(GL_R32UI, renderWidth, renderHeight, GL_NEAREST);
    filledReprojection          = make_shared<Texture>(GL_R32UI, renderWidth, renderHeight, GL_NEAREST);
    frameGenerationResult       = make_shared<Texture>(GL_RGBA8, presentationWidth, presentationHeight, GL_LINEAR);

    outputColor                 = nullptr;

    // LUTs
    sampleLut->loadLUT(resourcesDirectory + "lut.exr");
}

void OffscreenRenderer::execute()
{
    if (!enableInterpolation)
    {
        generatedFramesCount = 0;
    }
    currentOutputFrame = 0;
    isFirstCycleCompleted = false;
    jitterOffsetIndex = 0;
    for (currentInputFrame = startInputFrame; currentInputFrame < endInputFrame; currentInputFrame++)
    {
        for (int cycleFrameIndex = 0; cycleFrameIndex < generatedFramesCount + 1; cycleFrameIndex++, currentOutputFrame++)
        {
            if (cycleFrameIndex == 0)
            {
                // Rendered frame
                isRenderedFrame = true;
                isGeneratedFrame = false;

                // Load rendered frame
                load();

                jitterOffset = jitterSequence[jitterOffsetIndex];
                jitterOffsetIndex = (jitterOffsetIndex + 1) % jitterSequenceLength;
            }
            else
            {
                // Generated frame
                isRenderedFrame = false;
                isGeneratedFrame = true;
            }
            
            delta = static_cast<float>(cycleFrameIndex) / static_cast<float>(generatedFramesCount + 1);
            
            render();
            save();
        }

        if (!isFirstCycleCompleted)
        {
            if (enableInterpolation)
            {
                outputColor = nullptr;
                currentOutputFrame = 0;
            }
            isFirstCycleCompleted = true;
        }
    }
}

void OffscreenRenderer::load()
{
    // Load textures
    std::stringstream ss;
    ss << std::setw(4) << std::setfill('0') << currentInputFrame << ".png";
    std::string fileName = ss.str();

    GLenum sourceFormat = GL_RGB;
    GLenum sourceType = GL_UNSIGNED_BYTE;
    if(!enableSuperResolution)
    {
        inputColor->loadFromFile(inputHrColorDirectory + fileName, sourceFormat, sourceType);
    }
    else
    {
        rawInputHRColor->loadFromFile(inputHrColorDirectory + fileName, sourceFormat, sourceType);
    }
    
    sourceFormat = GL_RGBA;
    std::string depthDirectory = enableSuperResolution ? inputLrDepthDirectory : inputHrDepthDirectory;
    std::string motionVectorXDirectory = enableSuperResolution ? inputLrMotionVectorXDirectory : inputHrMotionVectorXDirectory;
    std::string motionVectorYDirectory = enableSuperResolution ? inputLrMotionVectorYDirectory : inputHrMotionVectorYDirectory;
    rawInputDepth->loadFromFile(depthDirectory + fileName, sourceFormat, sourceType);
    rawInputMotionVectorX->loadFromFile(motionVectorXDirectory + fileName, sourceFormat, sourceType);
    rawInputMotionVectorY->loadFromFile(motionVectorYDirectory + fileName, sourceFormat, sourceType);
}

void OffscreenRenderer::render()
{
    bindUniformBuffer();

    if (isRenderedFrame)
    {
        swapBuffers();
        processInputs();
        preprocess();

        if (enableSuperResolution)
        {
            if (!isFirstCycleCompleted)
            {
                // First frame
                upsampleFirstFrame();
                outputColor = currentHRColor;
            }
            else
            {
                superSample();
                outputColor = currentHRColor;
            }
        }
        if (enableInterpolation && isFirstCycleCompleted)
        {
            outputColor = previousHRColor;
        }
    }
    else if (isGeneratedFrame)
    {
        if (enableInterpolation && isFirstCycleCompleted)
        {
            interpolate();
            outputColor = frameGenerationResult;
        }
    }
}

void OffscreenRenderer::save()
{
    if (!outputColor)
    {
        return;
    }

    std::stringstream ss;
    ss << std::setw(4) << std::setfill('0') << currentOutputFrame << ".png";
    std::string fileName = ss.str();

    outputColor->saveAsPNG((outputDirectory + fileName).c_str(), 4);
}

void OffscreenRenderer::bindUniformBuffer()
{
    /*
    layout (binding = 10, std140) uniform cb_t
    {
                                        offset  size
        vec4 render_size;               0       16
        vec4 presentation_size;         16      16
        vec4 delta;                     32      16  
        vec2 jitter_offset;             48      8
        float depth_diff_threshold_sr;  56      4  
        float color_diff_threshold_fg;  60      4
        float depth_diff_threshold_fg;  64      4
        float depth_scale;              68      4
        float depth_bias;               72      4
        float render_scale;             76      4
    };
    // size = 80
    */

    UniformBlock uniformBlock;
    uniformBlock.render_size.x = static_cast<float>(renderWidth);
    uniformBlock.render_size.y = static_cast<float>(renderHeight);
    uniformBlock.render_size.z = 1.0f / static_cast<float>(renderWidth);
    uniformBlock.render_size.w = 1.0f / static_cast<float>(renderHeight);
    uniformBlock.presentation_size.x = static_cast<float>(presentationWidth);
    uniformBlock.presentation_size.y = static_cast<float>(presentationHeight);
    uniformBlock.presentation_size.z = 1.0f / static_cast<float>(presentationWidth);
    uniformBlock.presentation_size.w = 1.0f / static_cast<float>(presentationHeight);
    uniformBlock.delta.x = delta;
    uniformBlock.delta.y = delta * 0.5f;
    uniformBlock.delta.z = delta * 1.5f;
    uniformBlock.delta.w = delta * delta * 0.5f;
    uniformBlock.jitter_offset.x = jitterOffset.x;
    uniformBlock.jitter_offset.y = jitterOffset.y;
    uniformBlock.depth_diff_threshold_sr = depthDiffThresholdSR;
    uniformBlock.color_diff_threshold_fg = colorDiffThresholdFG;
    uniformBlock.depth_diff_threshold_fg = depthDiffThresholdFG;
    uniformBlock.depth_scale = depthScale;
    uniformBlock.depth_bias = depthBias;
    uniformBlock.render_scale = upsampleScale;
    
    constexpr int uniformBlockBindingPoint = 10;
    constexpr int uniformBlockSize = sizeof(UniformBlock);
    
    glGenBuffers(1, &uniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, uniformBlockSize, nullptr, GL_STATIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockBindingPoint, uniformBuffer, 0, uniformBlockSize);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, uniformBlockSize, &uniformBlock);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OffscreenRenderer::swapBuffers()
{
    if (enableInterpolation)
    {
        std::swap(currentHRColor, previousHRColor);
        std::swap(currentDilatedDepth, previousDilatedDepth);
        std::swap(currentDilatedMotionVector, previousDilatedMotionVector);
    }
    else 
    {
        if (enableSuperResolution)
        {
            std::swap(currentHRColor, previousHRColor);
            std::swap(currentDilatedDepth, previousDilatedDepth);
        }
    }
}

void OffscreenRenderer::processInputs()
{
    // Decode depths
    loadDepthCS->use();
    rawInputDepth->bindTexture(0);
    inputDepth->bindImageUnit(1, GL_WRITE_ONLY);
    loadDepthCS->dispatch(groupX_LR, groupY_LR, groupZ_LR);

    // Decode motion vectors
    loadMotionVectorCS->use();
    rawInputMotionVectorX->bindTexture(0);
    rawInputMotionVectorY->bindTexture(1);
    inputMotionVector->bindImageUnit(2, GL_WRITE_ONLY);
    loadMotionVectorCS->dispatch(groupX_LR, groupY_LR, groupZ_LR);

    // Sample HR color with jittered position to generate LR color
    if (enableSuperResolution)
    {
        loadLRColorCS->use();
        rawInputHRColor->bindTexture(0);
        inputColor->bindImageUnit(1, GL_WRITE_ONLY);
        loadLRColorCS->dispatch(groupX_LR, groupY_LR, groupZ_LR);
    }
}

void OffscreenRenderer::preprocess()
{
    // Dilate
    dilateCS->use();
    inputDepth->bindTexture(0);
    inputMotionVector->bindTexture(1);
    currentDilatedDepth->bindImageUnit(2, GL_WRITE_ONLY);
    currentDilatedMotionVector->bindImageUnit(3, GL_WRITE_ONLY);
    dilateCS->dispatch(groupX_LR, groupY_LR, groupZ_LR);

    // Copy inputColor -> currentHRColor
    if (enableInterpolation && !enableSuperResolution)
    {
        glCopyImageSubData(
            inputColor->getID(), GL_TEXTURE_2D, 0, 0, 0, 0,
            currentHRColor->getID(), GL_TEXTURE_2D, 0, 0, 0, 0,
            renderWidth, renderHeight, 1);
    }
}

void OffscreenRenderer::interpolate()
{
    clearCS->use();
    reprojection->bindImageUnit(0, GL_WRITE_ONLY);
    clearCS->dispatch(groupX_LR, groupY_LR, groupZ_LR);
    
    reprojectCS_I->use();
    currentDilatedDepth->bindTexture(0);
    currentDilatedMotionVector->bindTexture(1);
    previousDilatedMotionVector->bindTexture(2);
    reprojection->bindImageUnit(3, GL_READ_WRITE);
    reprojectCS_I->dispatch(groupX_LR, groupY_LR, groupZ_LR);
    
    fillCS->use();
    reprojection->bindTexture(0);
    filledReprojection->bindImageUnit(1, GL_WRITE_ONLY);
    fillCS->dispatch(groupX_LR, groupY_LR, groupZ_LR);
    
    warpCS_I->use();
    filledReprojection->bindTexture(0);
    currentHRColor->bindTexture(1);
    previousHRColor->bindTexture(2);
    currentDilatedDepth->bindTexture(3);
    previousDilatedDepth->bindTexture(4);
    currentDilatedMotionVector->bindTexture(5);
    previousDilatedMotionVector->bindTexture(6);
    frameGenerationResult->bindImageUnit(7, GL_WRITE_ONLY);
    sampleLut->bindTexture(8);
    warpCS_I->dispatch(groupX_HR, groupY_HR, groupZ_HR);
}

void OffscreenRenderer::upsampleFirstFrame()
{
    upsampleFirstFrameCS->use();
    inputColor->bindTexture(0);
    currentHRColor->bindImageUnit(1, GL_WRITE_ONLY);
    upsampleFirstFrameCS->dispatch(groupX_HR, groupY_HR, groupZ_HR);
}

void OffscreenRenderer::superSample()
{
    blendHistoryCS->use();
    inputColor->bindTexture(0);
    currentDilatedDepth->bindTexture(1);
    currentDilatedMotionVector->bindTexture(2);
    previousDilatedDepth->bindTexture(3);
    previousHRColor->bindTexture(4);
    currentHRColor->bindImageUnit(5, GL_WRITE_ONLY);
    sampleLut->bindTexture(6);
    blendHistoryCS->dispatch(groupX_HR, groupY_HR, groupZ_HR);
}
