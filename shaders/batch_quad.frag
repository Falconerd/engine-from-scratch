#version 330 core
out vec4 o_color;

in vec4 v_color;
in vec2 v_uvs;

uniform sampler2D texture_slot;

void main() {
	o_color = texture(texture_slot, v_uvs) * v_color;
}
