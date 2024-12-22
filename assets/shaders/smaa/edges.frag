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
layout(location = 1) in vec4 in_offsets[3];
layout(location = 0) out vec2 out_edges;
layout(binding = 0) uniform sampler2D tex_color;

void main() {
    out_edges = SMAAColorEdgeDetectionPS(in_texcoord, in_offsets, tex_color);
    // out_edges = SMAALumaEdgeDetectionPS(in_texcoord, in_offsets, tex_color);
    // out_edges = SMAA_DepthEdgeDetectionPS(in_texcoord, in_offsets, _);
}