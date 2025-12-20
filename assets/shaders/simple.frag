#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;

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
  vec3 normal = normalize(inNormal);
  vec3 lightDir = normalize(-global.lightDirection.xyz);

  // Simple diffuse lighting
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = diff * global.lightColor.rgb * global.lightIntensity;

  // Ambient
  vec3 ambient = vec3(0.1);

  vec3 result = (ambient + diffuse) * inColor.rgb;
  outColor = vec4(result, inColor.a);
}
