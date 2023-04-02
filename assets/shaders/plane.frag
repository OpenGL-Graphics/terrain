#version 460 core

// input coords passed from vertex shader
in vec2 texture_coord_vert;

// texture for plane
uniform sampler2D texture2d;

out vec4 color_out;

void main() {
  // texture for fragment
  color_out = texture(texture2d, texture_coord_vert);
}
