#version 450

layout(location=0) in vec3 inVec;
uniform mat4 cameraTransform;
uniform vec3 worldLocation;

mat4 matTranslate(vec3 v) {
	return mat4(
		1.0, 0.0, 0.0, v.x,
		0.0, 1.0, 0.0, v.y,
		0.0, 0.0, 1.0, v.z,
		0.0, 0.0, 0.0, 1.0
	);
}

void main() {
	gl_Position = vec4(inVec, 1.0) * matTranslate(worldLocation) * cameraTransform;
}
