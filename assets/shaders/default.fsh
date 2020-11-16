#version 450

in vec3 normal;

layout(location=0) out vec3 outColor;
uniform vec3 inColor;

void main() {
	outColor = (normal + vec3(1.0)) / 2.0;
}
