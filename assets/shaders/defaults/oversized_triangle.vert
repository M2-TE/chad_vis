#version 460

layout(location = 0) out vec2 out_uv;

// draw oversized triangle to achieve fullscreen fragment shader
void main() {
    int id = gl_VertexIndex;
    gl_Position.x = id == 1 ? 3.0 : -1.0;
    gl_Position.y = id == 2 ? 3.0 : -1.0;
    gl_Position.zw = vec2(0.0, 1.0);
    out_uv.x = id == 1 ? 2.0 : 0.0;
    out_uv.y = id == 2 ? 2.0 : 0.0;
}