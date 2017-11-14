#version 330
in vec4 vposition;
in vec4 vcolor;
in float plighting;
in float pblur;

uniform int renderingPoints;
uniform int onlyOpaque;

uniform vec3 lightAmbient;
uniform vec3 lightColor;
uniform vec3 lightDirection;
uniform float specTerm;

uniform vec3 eye;
uniform vec3 viewDirection;
uniform float theta;
uniform float phi;

out vec4 fragColor;

float PI = 3.1415926;

// Taken from www.neilmendoza.com/glsl-rotation-about-an-arbitrary-axis/
// All credit for function implementation goes to Neil Mendoza
mat4 rotateAboutAxis(vec3 axis, float angle)
{
	axis = normalize(axis);
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;

	return mat4(oc * axis.x * axis.x + c,			oc * axis.x * axis.y - axis.z * s, 	oc * axis.z * axis.x + axis.y * s, 	0.0,
				oc * axis.x * axis.y + axis.z * s, 	oc * axis.y * axis.y + c,			oc * axis.y * axis.z - axis.x * s, 	0.0,
				oc * axis.z * axis.x - axis.y * s,	oc * axis.y * axis.z + axis.x * s,	oc * axis.z * axis.z + c,			0.0,
				0.0,								0.0, 								0.0,								1.0);
}

void main() 
{
	if (renderingPoints == 1) { // Uses gl_PointCoord, which breaks OpenGL if fragment does not come from a GLPOINT
		// Make sure we render the particles in the correct order
		if (onlyOpaque == 1 && (vcolor.a < 1.0 || pblur > 0.0))
			discard;
		else if (onlyOpaque == 0 && vcolor.a == 1.0 && pblur == 0.0)
			discard;

		// Calculate normal
		// Taken from www.mmmovania.blogspot.com/2011/01/point-spirtes-as-spheres-in-opengl33.html
		vec3 N;
		N.xy = gl_PointCoord * 2.0 - 1.0;
		float mag = dot(N.xy, N.xy);
		if (mag > 1.0) discard;
		N.z = sqrt(1.0 - mag);
		N = normalize(N);

		float alpha = vcolor.a;
		if (pblur == 0.0) {
			// Antialias the edges
			// Inspired by www.desultoryquest.com/blog/drawing-anti-aliazed-circular-points-using-opengl-slash-webgl/
			float delta = fwidth(mag);
			alpha = alpha - smoothstep(1.02 - delta, 1.02 + delta, mag);
		}
		else {
			float delta = fwidth(pblur);
			alpha = alpha - smoothstep(1.0 - pblur, pblur, mag);
		}

		if (plighting > 0.5) { // Use lighting on the particle
			// Calculate lighting
			vec3 L = normalize(lightDirection);
			vec3 diffuseL = L;
			vec3 rayDirection = normalize(vposition.xyz - eye);

			// Rotate the lighting based on camera rotation
			mat4 rotationX = rotateAboutAxis(vec3(1.0, 0.0, 0.0), phi);
			mat4 rotationY = rotateAboutAxis(vec3(0.0, 1.0, 0.0), theta);
			diffuseL = (rotationX*rotationY*vec4(diffuseL, 0.0)).xyz;

			// Calculate diffuse lighting
			float diffuse = max(0.0, dot(diffuseL, N));

			// Calculate specular lighting and rotate it appropriately
			vec3 H = normalize(rayDirection + L);
			H = (rotationX*rotationY*vec4(H, 0.0)).xyz;
			float specular;
			if (specTerm < 0)
				specular = 0.0;
			else
				specular = pow(max(dot(N, H), 0.0), specTerm);

			fragColor = vcolor * vec4(lightColor, 1.0) * diffuse + vcolor * vec4(lightAmbient, 1.0) + vec4(1.0) * specular;
			fragColor = vec4(clamp(fragColor, 0.0, 1.0).xyz, alpha);
		}
		else { // Don't use lighting on the particle
			fragColor = vec4(vcolor.xyz, alpha);
		}
	}
	else {
		vec3 N;
		if (gl_FrontFacing)
			N = vec3(0.0, 1.0, 0.0);
		else
			N = vec3(0.0, -1.0, 0.0);
		vec3 L = -lightDirection;
		float diffuse = max(0.0, dot(L, N));

		vec3 H = normalize(-viewDirection + L);
		float specular;
		if (specTerm < 0)
			specular = 0.0;
		else
			specular = pow(max(dot(N, H), 0.0), specTerm);
		
		fragColor = vcolor * vec4(lightColor, 1.0) * diffuse + vcolor * vec4(lightAmbient, 1.0) + vec4(1.0) * specular;
		fragColor = clamp(fragColor, 0.0, 1.0);
	}
}
