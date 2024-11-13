///// Packing /////
// Packing constants
const uint depthBits = 11;
const uint xBits = 11;
const uint yBits = 10;

const uint maxDepth = (1 << depthBits) - 1;
const uint minX = -(1 << (xBits - 1));
const uint maxX =  (1 << (xBits - 1)) - 1;
const uint minY = -(1 << (yBits - 1));
const uint maxY =  (1 << (yBits - 1)) - 1;

// Pack (depth, relativePos.xy) to 11/11/10 uint
// Depth precision: 0.0004882
// RelativePos.x range: [-1024, 1023]
// RelativePos.y range: [-512, 511]
uint packReprojectionDataToUint(float depth, ivec2 sourcePos, ivec2 targetPos)
{
    uint uDepth = uint(float(maxDepth) * depth);
    ivec2 relativePos = clamp(targetPos - sourcePos, ivec2(minX, minY), ivec2(maxX, maxY));
    uvec2 uRelativePos = uvec2(relativePos - ivec2(minX, minY));

    uint result = (uDepth << (32 - depthBits)) | (uRelativePos.x << yBits) | (uRelativePos.y);

    return result;
}

float unpackDepthFromUint(uint reprojectionData)
{
    uint uDepth = reprojectionData >> (32 - depthBits);
    return float(uDepth) / float(maxDepth);
}

ivec2 unpackSourcePosFromUint(uint reprojectionData, ivec2 targetPos)
{
    uint uRelativeX = (reprojectionData >> yBits) & uint((1 << xBits) - 1);
    uint uRelativeY = reprojectionData & uint((1 << yBits) - 1);
    ivec2 relativePos = ivec2(uRelativeX, uRelativeY) + ivec2(minX, minY);
    ivec2 sourcePos = targetPos - relativePos;
    return sourcePos;
}