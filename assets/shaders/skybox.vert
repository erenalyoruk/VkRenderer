#version 450

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outTexCoord;

layout(set = 0, binding = 0) uniform GlobalUniforms {
  mat4 viewProjection;
  mat4 view;
  mat4 projection;
  vec4 cameraPosition;
  vec4 lightDirection;
  vec4 lightColor;
  float lightIntensity;
  float time;
}
global;

void main() {
  // Extract rotation from view matrix (remove translation)
  mat3 rotationOnly = mat3(global.view);
  vec3 rotatedPos = rotationOnly * inPosition;

  vec4 pos = global.projection * vec4(rotatedPos, 1.0);

  // Set z = w to ensure skybox is always at maximum depth
  gl_Position = pos.xyww;

  // Use local position as cubemap direction
  outTexCoord = inPosition;
}
