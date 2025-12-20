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
  // Calculate distance from camera for depth-based coloring
  float dist = length(global.cameraPosition.xyz - inWorldPos);

  // Fade wireframe color based on distance
  float fade = clamp(1.0 - dist / 100.0, 0.2, 1.0);

  // Green wireframe color
  vec3 wireColor = vec3(0.0, 1.0, 0.3) * fade;

  outColor = vec4(wireColor, 1.0);
}
