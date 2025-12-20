#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in mat3 inTBN;

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
layout(set = 1, binding = 2) uniform sampler2D normalTex;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessTex;
layout(set = 1, binding = 4) uniform sampler2D occlusionTex;
layout(set = 1, binding = 5) uniform sampler2D emissiveTex;

const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
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

// Geometry Function (Schlick-GGX)
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

// Fresnel (Schlick approximation)
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
  // Sample textures
  vec4 baseColor =
      texture(baseColorTex, inTexCoord) * material.baseColorFactor * inColor;

  // Alpha cutoff for masked materials
  if (baseColor.a < material.alphaCutoff) {
    discard;
  }

  vec2 metallicRoughness = texture(metallicRoughnessTex, inTexCoord).bg;
  float metallic = metallicRoughness.x * material.metallicFactor;
  float roughness = metallicRoughness.y * material.roughnessFactor;
  roughness = max(roughness, 0.04);  // Prevent divide by zero

  // Sample occlusion (stored in R channel typically)
  float ao = texture(occlusionTex, inTexCoord).r;
  ao = mix(1.0, ao, material.occlusionStrength);

  // Sample emissive
  vec3 emissive =
      texture(emissiveTex, inTexCoord).rgb * material.emissiveFactor;

  // Sample and transform normal from tangent space to world space
  vec3 normalSample = texture(normalTex, inTexCoord).rgb;
  normalSample = normalSample * 2.0 - 1.0;  // Unpack from [0,1] to [-1,1]
  vec3 N = normalize(inTBN * normalSample);

  vec3 V = normalize(global.cameraPosition.xyz - inWorldPos);
  vec3 L = normalize(-global.lightDirection.xyz);
  vec3 H = normalize(V + L);

  // Calculate reflectance at normal incidence
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, baseColor.rgb, metallic);

  // Cook-Torrance BRDF
  float NDF = DistributionGGX(N, H, roughness);
  float G = GeometrySmith(N, V, L, roughness);
  vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

  vec3 numerator = NDF * G * F;
  float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
  vec3 specular = numerator / denominator;

  // Energy conservation
  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;

  // Lambertian diffuse
  float NdotL = max(dot(N, L), 0.0);
  vec3 diffuse = kD * baseColor.rgb / PI;

  // Final color
  vec3 radiance = global.lightColor.rgb * global.lightIntensity;
  vec3 Lo = (diffuse + specular) * radiance * NdotL;

  // Ambient with occlusion
  vec3 ambient = vec3(0.03) * baseColor.rgb * ao;

  // Combine lighting + emissive
  vec3 color = ambient + Lo + emissive;

  // HDR tonemapping (Reinhard)
  color = color / (color + vec3(1.0));

  // Gamma correction
  color = pow(color, vec3(1.0 / 2.2));

  outColor = vec4(color, baseColor.a);
}
