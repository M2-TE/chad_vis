#version 460

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec4 out_color;

vec3 linear_to_gamma(vec3 color) {
    return pow(color, vec3(1.0 / 2.2));
}
vec3 gamma_to_linear(vec3 color) {
    return pow(color, vec3(2.2));
}

void main() {
    vec3 light_dir = normalize(vec3(0.0, 3.0, -3.0));
    float intensity = max(dot(in_normal, light_dir), 0.0);
    vec3 color = in_color * intensity;
    // color = linear_to_gamma(color);
    // color = gamma_to_linear(color);
    out_color = vec4(color, 1.0);
}