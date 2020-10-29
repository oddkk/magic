#version 450
layout(location=0) in vec2 inVec;
uniform vec2 inPosition;
uniform vec2 inScale;
uniform vec2 inSize;
uniform float inZOffset;

void main() {
	gl_Position = vec4(((inVec * inSize) + inPosition) * inScale, inZOffset, 1.0);
}
