#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inColor;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec4 outColor;
layout(location = 2) flat out uint outMaterialIndex;

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

struct ObjectData {
  mat4 model;
  mat4 normalMatrix;
  vec4 boundingSphere;
  uint materialIndex;
  uint indexCount;
  uint indexOffset;
  int vertexOffset;
};

layout(std430, set = 2, binding = 0) readonly buffer ObjectBuffer {
  ObjectData objects[];
};

void main() {
  ObjectData obj = objects[gl_InstanceIndex];

  vec4 worldPos = obj.model * vec4(inPosition, 1.0);
  gl_Position = global.viewProjection * worldPos;

  outTexCoord = inTexCoord;
  outColor = inColor;
  outMaterialIndex = obj.materialIndex;
}
