///// Sample /////
mediump ivec2 clampCoord(mediump ivec2 pos, mediump ivec2 offset, mediump ivec2 textureSize)
{
    mediump ivec2 result = pos + offset;
    result.x = int((offset.x < 0) ? max(result.x, 0) : result.x);
    result.x = int((offset.x > 0) ? min(result.x, textureSize.x - 1) : result.x);
    result.y = int((offset.y < 0) ? max(result.y, 0) : result.y);
    result.y = int((offset.y > 0) ? min(result.y, textureSize.y - 1) : result.y);
    return result;
}

mediump vec4 sampleWithLut(in sampler2D tex, vec2 uv, vec2 textureSize, in sampler2D lut)
{
    vec2 fPos = uv * textureSize;
    vec2 center = round(fPos);
    vec2 d = fPos - center + vec2(0.5, 0.5);

    // LUT: (32 * 4) * (32 * 4) = 128 * 128
    // 0.25 = 32 / 128
    mediump vec2 lutSampleUV = (31 * d + vec2(0.5, 0.5)) / 128.0f;
    mediump float weight00 = texture(lut, lutSampleUV).x;
    mediump float weight01 = texture(lut, lutSampleUV + vec2(0, 1) * 0.25).x;
    mediump float weight02 = texture(lut, lutSampleUV + vec2(0, 2) * 0.25).x;
    mediump float weight03 = texture(lut, lutSampleUV + vec2(0, 3) * 0.25).x;
    mediump float weight10 = texture(lut, lutSampleUV + vec2(1, 0) * 0.25).x;
    mediump float weight11 = texture(lut, lutSampleUV + vec2(1, 1) * 0.25).x;
    mediump float weight12 = texture(lut, lutSampleUV + vec2(1, 2) * 0.25).x;
    mediump float weight13 = texture(lut, lutSampleUV + vec2(1, 3) * 0.25).x;
    mediump float weight20 = texture(lut, lutSampleUV + vec2(2, 0) * 0.25).x;
    mediump float weight21 = texture(lut, lutSampleUV + vec2(2, 1) * 0.25).x;
    mediump float weight22 = texture(lut, lutSampleUV + vec2(2, 2) * 0.25).x;
    mediump float weight23 = texture(lut, lutSampleUV + vec2(2, 3) * 0.25).x;
    mediump float weight30 = texture(lut, lutSampleUV + vec2(3, 0) * 0.25).x;
    mediump float weight31 = texture(lut, lutSampleUV + vec2(3, 1) * 0.25).x;
    mediump float weight32 = texture(lut, lutSampleUV + vec2(3, 2) * 0.25).x;
    mediump float weight33 = texture(lut, lutSampleUV + vec2(3, 3) * 0.25).x;

    mediump ivec2 samplePos11 = ivec2(center - vec2(0.5, 0.5));
    mediump ivec2 iTextureSize = ivec2(textureSize);
    mediump vec4 color00 = texelFetch(tex, clampCoord(samplePos11, ivec2(-1, -1), iTextureSize), 0);
    mediump vec4 color01 = texelFetch(tex, clampCoord(samplePos11, ivec2(-1, 0), iTextureSize), 0);
    mediump vec4 color02 = texelFetch(tex, clampCoord(samplePos11, ivec2(-1, 1), iTextureSize), 0);
    mediump vec4 color03 = texelFetch(tex, clampCoord(samplePos11, ivec2(-1, 2), iTextureSize), 0);
    mediump vec4 color10 = texelFetch(tex, clampCoord(samplePos11, ivec2(0, -1), iTextureSize), 0);
    mediump vec4 color11 = texelFetch(tex, clampCoord(samplePos11, ivec2(0, 0), iTextureSize), 0);
    mediump vec4 color12 = texelFetch(tex, clampCoord(samplePos11, ivec2(0, 1), iTextureSize), 0);
    mediump vec4 color13 = texelFetch(tex, clampCoord(samplePos11, ivec2(0, 2), iTextureSize), 0);
    mediump vec4 color20 = texelFetch(tex, clampCoord(samplePos11, ivec2(1, -1), iTextureSize), 0);
    mediump vec4 color21 = texelFetch(tex, clampCoord(samplePos11, ivec2(1, 0), iTextureSize), 0);
    mediump vec4 color22 = texelFetch(tex, clampCoord(samplePos11, ivec2(1, 1), iTextureSize), 0);
    mediump vec4 color23 = texelFetch(tex, clampCoord(samplePos11, ivec2(1, 2), iTextureSize), 0);
    mediump vec4 color30 = texelFetch(tex, clampCoord(samplePos11, ivec2(2, -1), iTextureSize), 0);
    mediump vec4 color31 = texelFetch(tex, clampCoord(samplePos11, ivec2(2, 0), iTextureSize), 0);
    mediump vec4 color32 = texelFetch(tex, clampCoord(samplePos11, ivec2(2, 1), iTextureSize), 0);
    mediump vec4 color33 = texelFetch(tex, clampCoord(samplePos11, ivec2(2, 2), iTextureSize), 0);

    mediump vec4 result = vec4(0, 0, 0, 0);
    result += color00 * weight00;
    result += color01 * weight01;
    result += color02 * weight02;
    result += color03 * weight03;
    result += color10 * weight10;
    result += color11 * weight11;
    result += color12 * weight12;
    result += color13 * weight13;
    result += color20 * weight20;
    result += color21 * weight21;
    result += color22 * weight22;
    result += color23 * weight23;
    result += color30 * weight30;
    result += color31 * weight31;
    result += color32 * weight32;
    result += color33 * weight33;
    result /= (weight00 + weight01 + weight02 + weight03 + weight10 + weight11 + weight12 + weight13 + weight20 + weight21 + weight22 + weight23 + weight30 + weight31 + weight32 + weight33);
    return result;
}