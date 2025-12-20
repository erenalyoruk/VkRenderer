#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in mat3 inTBN;
layout(location = 7) flat in uint inMaterialIndex;

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

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;
  return nom / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;
  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;
  return nom / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);
  return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
  MaterialData mat = materials[inMaterialIndex];

  vec4 baseColor =
      texture(textures[nonuniformEXT(mat.baseColorTexIdx)], inTexCoord) *
      mat.baseColorFactor * inColor;

  float alphaCutoff = mat.roughnessAlphaCutoffOcclusion.y;
  if (baseColor.a < alphaCutoff) {
    discard;
  }

  vec2 metallicRoughness =
      texture(textures[nonuniformEXT(mat.metallicRoughnessTexIdx)], inTexCoord)
          .bg;
  float metallic = metallicRoughness.x * mat.emissiveFactorAndMetallic.w;
  float roughness = metallicRoughness.y * mat.roughnessAlphaCutoffOcclusion.x;
  roughness = max(roughness, 0.04);

  float ao =
      texture(textures[nonuniformEXT(mat.occlusionTexIdx)], inTexCoord).r;
  float occlusionStrength = mat.roughnessAlphaCutoffOcclusion.z;
  ao = mix(1.0, ao, occlusionStrength);

  vec3 emissive =
      texture(textures[nonuniformEXT(mat.emissiveTexIdx)], inTexCoord).rgb *
      mat.emissiveFactorAndMetallic.xyz;

  vec3 normalSample =
      texture(textures[nonuniformEXT(mat.normalTexIdx)], inTexCoord).rgb;
  normalSample = normalSample * 2.0 - 1.0;
  vec3 N = normalize(inTBN * normalSample);

  vec3 V = normalize(global.cameraPosition.xyz - inWorldPos);
  vec3 L = normalize(-global.lightDirection.xyz);
  vec3 H = normalize(V + L);

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, baseColor.rgb, metallic);

  float NDF = DistributionGGX(N, H, roughness);
  float G = GeometrySmith(N, V, L, roughness);
  vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

  vec3 numerator = NDF * G * F;
  float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
  vec3 specular = numerator / denominator;

  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;

  float NdotL = max(dot(N, L), 0.0);
  vec3 diffuse = kD * baseColor.rgb / PI;

  vec3 radiance = global.lightColor.rgb * global.lightIntensity;
  vec3 Lo = (diffuse + specular) * radiance * NdotL;

  vec3 ambient = vec3(0.03) * baseColor.rgb * ao;
  vec3 color = ambient + Lo + emissive;

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  outColor = vec4(color, baseColor.a);
}
