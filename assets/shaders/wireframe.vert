#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec3 outWorldPos;

layout(set = 0, binding = 0) uniform GlobalUniforms {
  mat4 viewProjection;
  vec4 cameraPosition;
  vec4 lightDirection;
  vec4 lightColor;
  float lightIntensity;
  float time;
}
global;

layout(push_constant) uniform ObjectUniforms {
  mat4 model;
  mat4 normalMatrix;
}
object;

void main() {
  vec4 worldPos = object.model * vec4(inPosition, 1.0);
  gl_Position = global.viewProjection * worldPos;
  outWorldPos = worldPos.xyz;
}
