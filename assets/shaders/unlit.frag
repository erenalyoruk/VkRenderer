#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialUniforms {
  vec4 baseColorFactor;
  float metallicFactor;
  float roughnessFactor;
  float alphaCutoff;
  float _padding;
}
material;

layout(set = 1, binding = 1) uniform sampler2D baseColorTex;

void main() {
  vec4 texColor = texture(baseColorTex, inTexCoord);
  vec4 finalColor = texColor * material.baseColorFactor * inColor;

  // Alpha cutoff
  if (finalColor.a < material.alphaCutoff) {
    discard;
  }

  // Gamma correction
  finalColor.rgb = pow(finalColor.rgb, vec3(1.0 / 2.2));

  outColor = finalColor;
}
