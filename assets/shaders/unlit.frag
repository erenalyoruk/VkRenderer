#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialUniforms {
  vec4 baseColorFactor;
  vec3 emissiveFactor;
  float metallicFactor;
  float roughnessFactor;
  float alphaCutoff;
  float occlusionStrength;
  float _padding;
}
material;

layout(set = 1, binding = 1) uniform sampler2D baseColorTex;
layout(set = 1, binding = 2) uniform sampler2D normalTex;             // unused
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessTex;  // unused
layout(set = 1, binding = 4) uniform sampler2D occlusionTex;          // unused
layout(set = 1, binding = 5) uniform sampler2D emissiveTex;

void main() {
  vec4 texColor = texture(baseColorTex, inTexCoord);
  vec4 finalColor = texColor * material.baseColorFactor * inColor;

  // Alpha cutoff
  if (finalColor.a < material.alphaCutoff) {
    discard;
  }

  // Add emissive
  vec3 emissive =
      texture(emissiveTex, inTexCoord).rgb * material.emissiveFactor;
  finalColor.rgb += emissive;

  // Gamma correction
  finalColor.rgb = pow(finalColor.rgb, vec3(1.0 / 2.2));

  outColor = finalColor;
}
