#version 450

layout(location = 0) in vec3 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 3, binding = 0) uniform samplerCube skyboxCube;

void main() {
  vec3 color = texture(skyboxCube, inTexCoord).rgb;

  // Simple tonemap
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  outColor = vec4(color, 1.0);
}
