///// Uniforms /////
layout (binding = 10, std140) uniform cb_t
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
} cb;