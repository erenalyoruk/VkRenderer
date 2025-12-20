#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inColor;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec4 outColor;
layout(location = 4) out mat3 outTBN;

layout(set = 0, binding = 0) uniform GlobalUniforms {
  mat4 viewProjection;
  vec4 cameraPosition;
  vec4 lightDirection;
  vec4 lightColor;
  float lightIntensity;
  float time;
}
global;

layout(push_constant) uniform ObjectUniforms {
  mat4 model;
  mat4 normalMatrix;
}
object;

void main() {
  vec4 worldPos = object.model * vec4(inPosition, 1.0);
  gl_Position = global.viewProjection * worldPos;

  outWorldPos = worldPos.xyz;

  // Transform normal and tangent to world space
  mat3 normalMat = mat3(object.normalMatrix);
  vec3 N = normalize(normalMat * inNormal);
  vec3 T = normalize(normalMat * inTangent.xyz);

  // Re-orthogonalize T with respect to N
  T = normalize(T - dot(T, N) * N);

  // Calculate bitangent with handedness from tangent.w
  vec3 B = cross(N, T) * inTangent.w;

  outTBN = mat3(T, B, N);
  outNormal = N;
  outTexCoord = inTexCoord;
  outColor = inColor;
}
