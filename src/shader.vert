#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SceneData {
    mat4 mvpMatrix[2];
} sceneData;

layout(push_constant) uniform ObjectData {
    int id;
} objectData;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexUV;   // テクスチャ座標入力
layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragmentTexUV;    // フラグメントシェーダへのテクスチャ座標出力


void main() {
    gl_Position = sceneData.mvpMatrix[objectData.id] * vec4(inPos, 1.0);
    fragmentColor = inColor;
    fragmentTexUV = inTexUV;    // テクスチャ座標
}