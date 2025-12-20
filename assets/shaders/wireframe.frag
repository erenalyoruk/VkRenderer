#version 450

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUniforms {
  mat4 viewProjection;
  vec4 cameraPosition;
  vec4 lightDirection;
  vec4 lightColor;
  float lightIntensity;
  float time;
}
global;

void main() {
  float dist = length(global.cameraPosition.xyz - inWorldPos);
  float fade = clamp(1.0 - dist / 100.0, 0.2, 1.0);
  vec3 wireColor = vec3(0.0, 1.0, 0.3) * fade;
  outColor = vec4(wireColor, 1.0);
}
