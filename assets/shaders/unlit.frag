#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec4 inColor;
layout(location = 2) flat in uint inMaterialIndex;

layout(location = 0) out vec4 outColor;

struct MaterialData {
  vec4 baseColorFactor;
  vec4 emissiveFactorAndMetallic;
  vec4 roughnessAlphaCutoffOcclusion;
  uint baseColorTexIdx;
  uint normalTexIdx;
  uint metallicRoughnessTexIdx;
  uint occlusionTexIdx;
  uint emissiveTexIdx;
  uint _padding[3];
};

layout(std430, set = 1, binding = 0) readonly buffer MaterialBuffer {
  MaterialData materials[];
};

layout(set = 1, binding = 1) uniform sampler2D textures[];

void main() {
  MaterialData mat = materials[inMaterialIndex];

  vec4 texColor =
      texture(textures[nonuniformEXT(mat.baseColorTexIdx)], inTexCoord);
  vec4 finalColor = texColor * mat.baseColorFactor * inColor;

  float alphaCutoff = mat.roughnessAlphaCutoffOcclusion.y;
  if (finalColor.a < alphaCutoff) {
    discard;
  }

  vec3 emissive =
      texture(textures[nonuniformEXT(mat.emissiveTexIdx)], inTexCoord).rgb *
      mat.emissiveFactorAndMetallic.xyz;
  finalColor.rgb += emissive;

  finalColor.rgb = pow(finalColor.rgb, vec3(1.0 / 2.2));

  outColor = finalColor;
}
