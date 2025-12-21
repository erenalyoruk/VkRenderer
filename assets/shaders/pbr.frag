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
  mat4 view;
  mat4 projection;
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

// IBL resources
layout(set = 3, binding = 0) uniform samplerCube skyboxCube;
layout(set = 3, binding = 1) uniform samplerCube irradianceCube;
layout(set = 3, binding = 2) uniform samplerCube prefilteredCube;
layout(set = 3, binding = 3) uniform sampler2D brdfLUT;

// Forward+ lighting (set 4)
struct GPULight {
  vec4 positionAndRadius;
  vec4 colorAndIntensity;
  vec4 directionAndType;
  vec4 spotParams;
};

layout(std430, set = 4, binding = 0) readonly buffer LightBuffer {
  GPULight lights[];
};

layout(std430, set = 4, binding = 1) readonly buffer LightIndexBuffer {
  uint lightIndices[];
};

layout(std430, set = 4, binding = 2) readonly buffer LightGridBuffer {
  uvec2 lightGrid[];  // x = offset, y = count
};

layout(set = 4, binding = 3) uniform LightCullUniforms {
  mat4 lightView;
  mat4 lightProjection;
  mat4 lightInvProjection;
  uvec4 screenDimensions;
  uint lightCount;
  float nearPlane;
  float farPlane;
  uint _lightPadding;
}
lightCull;

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;
const uint TILE_SIZE = 16;
const uint MAX_LIGHTS_PER_TILE = 256;

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

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) *
                  pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Calculate attenuation for point/spot lights
float getDistanceAttenuation(float distance, float radius) {
  float d = distance / radius;
  float d2 = d * d;
  float d4 = d2 * d2;
  float falloff = clamp(1.0 - d4, 0.0, 1.0);
  return (falloff * falloff) / (distance * distance + 1.0);
}

// Calculate spot light cone attenuation
float getSpotAttenuation(vec3 L, vec3 spotDir, float cosInner, float cosOuter) {
  float cosAngle = dot(-L, spotDir);
  return clamp((cosAngle - cosOuter) / (cosInner - cosOuter), 0.0, 1.0);
}

// Calculate lighting contribution from a single light
vec3 calculateLightContribution(GPULight light, vec3 N, vec3 V, vec3 worldPos,
                                vec3 albedo, float metallic, float roughness,
                                vec3 F0) {
  vec3 lightPos = light.positionAndRadius.xyz;
  float radius = light.positionAndRadius.w;
  vec3 lightColor = light.colorAndIntensity.xyz;
  float intensity = light.colorAndIntensity.w;
  uint lightType = uint(light.directionAndType.w);

  vec3 L = normalize(lightPos - worldPos);
  float distance = length(lightPos - worldPos);

  // Skip if outside radius
  if (distance > radius) {
    return vec3(0.0);
  }

  vec3 H = normalize(V + L);

  // Distance attenuation
  float attenuation = getDistanceAttenuation(distance, radius);

  // Spot light cone attenuation
  if (lightType == 1) {  // Spot light
    vec3 spotDir = normalize(light.directionAndType.xyz);
    float cosInner = light.spotParams.x;
    float cosOuter = light.spotParams.y;
    attenuation *= getSpotAttenuation(L, spotDir, cosInner, cosOuter);
  }

  vec3 radiance = lightColor * intensity * attenuation;

  // Cook-Torrance BRDF
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
  return (kD * albedo / PI + specular) * radiance * NdotL;
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
  vec3 R = reflect(-V, N);

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, baseColor.rgb, metallic);

  vec3 Lo = vec3(0.0);

  // Directional light (global)
  {
    vec3 L = normalize(-global.lightDirection.xyz);
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator =
        4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 radiance = global.lightColor.rgb * global.lightIntensity;
    Lo += (kD * baseColor.rgb / PI + specular) * radiance * NdotL;
  }

  // Forward+ tiled lighting
  if (lightCull.lightCount > 0) {
    // Get tile index from screen position
    ivec2 tileId = ivec2(gl_FragCoord.xy) / ivec2(TILE_SIZE);
    uint tileIndex =
        uint(tileId.y) * lightCull.screenDimensions.z + uint(tileId.x);

    uvec2 gridData = lightGrid[tileIndex];
    uint lightOffset = gridData.x;
    uint lightCount = gridData.y;

    // Iterate through lights affecting this tile
    for (uint i = 0; i < lightCount && i < MAX_LIGHTS_PER_TILE; ++i) {
      uint lightIndex = lightIndices[lightOffset + i];
      GPULight light = lights[lightIndex];

      Lo += calculateLightContribution(light, N, V, inWorldPos, baseColor.rgb,
                                       metallic, roughness, F0);
    }
  }

  // IBL - Ambient lighting
  const float IBL_INTENSITY = 0.3;

  vec3 F_ibl = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
  vec3 kS_ibl = F_ibl;
  vec3 kD_ibl = 1.0 - kS_ibl;
  kD_ibl *= 1.0 - metallic;

  vec3 irradiance = texture(irradianceCube, N).rgb;
  vec3 diffuse_ibl = irradiance * baseColor.rgb;

  vec3 prefilteredColor =
      textureLod(prefilteredCube, R, roughness * MAX_REFLECTION_LOD).rgb;
  vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
  vec3 specular_ibl = prefilteredColor * (F_ibl * brdf.x + brdf.y);

  vec3 ambient = (kD_ibl * diffuse_ibl + specular_ibl) * ao * IBL_INTENSITY;

  vec3 color = ambient + Lo + emissive;

  // Tonemap and gamma correct
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  outColor = vec4(color, baseColor.a);
}
