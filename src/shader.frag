#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform sampler2D texSampler; // テクスチャサンプラ

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentTexUV;
layout(location = 0) out vec4 outColor;

void main() {
    // outColor = vec4(fragmentColor, 1.0);
    outColor = texture(texSampler, fragmentTexUV);  // テクスチャサンプリング
}