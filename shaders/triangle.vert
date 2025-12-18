#version 460

void main() {
  const vec3 positions[3] =
      vec3[](vec3(0.0, 0.5, 0.0), vec3(-0.5, -0.5, 0.0), vec3(0.5, -0.5, 0.0));

  const vec3 colors[3] =
      vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

  gl_Position = vec4(positions[gl_VertexIndex], 1.0);
}
