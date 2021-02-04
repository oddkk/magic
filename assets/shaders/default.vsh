#version 450

layout(location=0) in vec3 inVec;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inColor;

uniform mat4 cameraTransform;
uniform mat4 normalTransform;
uniform mat4 worldTransform;

varying vec3 normalInterp;
varying vec3 vertPos;
out vec3 color;

void main() {
	gl_Position = vec4(inVec, 1.0) * worldTransform * cameraTransform;
	vec4 vertPos4 = vec4(inVec, 1.0) * worldTransform;
	vertPos = vec3(vertPos4) / vertPos4.w;
	normalInterp = vec3(vec4(inNormal, 0.0) * normalTransform);
	color = inColor;
}
