#version 460
#extension GL_ARB_shading_language_include: require
#extension GL_EXT_control_flow_attributes: require
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0
layout(constant_id = 0) const float SMAA_RT_METRICS_X = 1.0 / 1280.0;
layout(constant_id = 1) const float SMAA_RT_METRICS_Y = 1.0 / 720.0;
layout(constant_id = 2) const float SMAA_RT_METRICS_Z = 1280.0;
layout(constant_id = 3) const float SMAA_RT_METRICS_W = 720.0;
const vec4 SMAA_RT_METRICS = vec4(SMAA_RT_METRICS_X, SMAA_RT_METRICS_Y, SMAA_RT_METRICS_Z, SMAA_RT_METRICS_W);
#include "smaa/settings.glsl"

layout(location = 0) out vec2 out_texcoord;
layout(location = 1) out vec2 out_pixcoord;
layout(location = 2) out vec4 out_offsets[3];

void main() {
    // oversized triangle
    int id = gl_VertexIndex;
    gl_Position.x = id == 1 ? 3.0 : -1.0;
    gl_Position.y = id == 2 ? 3.0 : -1.0;
    gl_Position.zw = vec2(0.0, 1.0);
    out_texcoord.x = id == 1 ? 2.0 : 0.0;
    out_texcoord.y = id == 2 ? 2.0 : 0.0;
    SMAABlendingWeightCalculationVS(out_texcoord, out_pixcoord, out_offsets);
}