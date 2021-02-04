#version 450

in vec3 normalInterp;
in vec3 vertPos;
in vec3 color;

layout(location=0) out vec4 outColor;
uniform vec3 inLightPos;

const vec3 lightColor = vec3(1);
const vec3 ambientLightColor = vec3(0.05);
const float lightPower = 80.0;
const vec3 specularColor = vec3(0.5);
const float shininess = 16.0;
const float screenGamma = 2.2;

void main() {
	vec3 normal = normalize(normalInterp);
	vec3 lightDir = inLightPos - vertPos;

	float distance = length(lightDir);
	distance = distance * distance;

	lightDir = normalize(lightDir);

	float lambertian = max(dot(lightDir, normal), 0.0);
	float specular = 0;

	if (lambertian > 0.0) {
		vec3 viewDir = normalize(-vertPos);

		vec3 halfDir = normalize(lightDir + viewDir);
		float specAngle = max(dot(halfDir, normal), 0.0);
		specular = pow(specAngle, shininess);
	}

	vec3 diffuseColor = color;

	vec3 colorLinear =
		color * ambientLightColor +
		diffuseColor * lambertian * lightColor * lightPower / distance +
		specularColor * specular * lightColor * lightPower / distance;

	vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0 / screenGamma));

	outColor = vec4(colorGammaCorrected, 1.0);
}
