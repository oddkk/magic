#version 450
layout(location=0) out vec3 outColor;
uniform vec3 inColor;

void main() {
	outColor = inColor;
}
