#version 330 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_uvs;
layout (location = 2) in vec4 a_color;

out vec4 v_color;
out vec2 v_uvs;

uniform mat4 projection;

void main() {
	v_color = a_color;
	v_uvs = a_uvs;
	gl_Position = projection * vec4(a_pos, 0.0, 1.0);
}
