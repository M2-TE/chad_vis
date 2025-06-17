#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 0) out vec4 out_color;

void main() {
    vec3 light_pos = vec3(0.0, 3.0, 0.0);
    vec3 light_dir = normalize(in_position - light_pos);
    float intensity = max(dot(in_normal, light_dir), 0.0);
    vec3 color = in_color * intensity;
    out_color = vec4(color, 1.0);
}