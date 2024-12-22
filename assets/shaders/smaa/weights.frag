#version 460
#extension GL_ARB_shading_language_include: require
#extension GL_EXT_control_flow_attributes: require
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
layout(constant_id = 0) const float SMAA_RT_METRICS_X = 1.0 / 1280.0;
layout(constant_id = 1) const float SMAA_RT_METRICS_Y = 1.0 / 720.0;
layout(constant_id = 2) const float SMAA_RT_METRICS_Z = 1280.0;
layout(constant_id = 3) const float SMAA_RT_METRICS_W = 720.0;
const vec4 SMAA_RT_METRICS = vec4(SMAA_RT_METRICS_X, SMAA_RT_METRICS_Y, SMAA_RT_METRICS_Z, SMAA_RT_METRICS_W);
#include "smaa/settings.glsl"

layout(location = 0) in vec2 in_texcoord;
layout(location = 1) in vec2 in_pixcoord;
layout(location = 2) in vec4 in_offsets[3];
layout(location = 0) out vec4 out_weights;
layout(binding = 0) uniform sampler2D tex_area;
layout(binding = 1) uniform sampler2D tex_search;
layout(binding = 2) uniform sampler2D tex_edges;

void main() {
    out_weights = SMAABlendingWeightCalculationPS(
        in_texcoord, in_pixcoord, in_offsets, 
        tex_edges, tex_area, tex_search, 
        vec4(0, 0, 0, 0));
}