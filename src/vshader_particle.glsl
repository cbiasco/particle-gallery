#version 330
in vec4 vertex_position;
in vec4 vertex_color;
in float particle_lighting;
in float particle_size;
in float particle_blur;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform vec3 eye;

out vec4 vposition;
out vec4 vcolor;
out float plighting;
out float pblur;

void main()  {
	gl_Position = P*M*V*vertex_position;

	float dist = distance(eye, vertex_position.xyz);
	gl_PointSize = max(1.0, particle_size/dist);
	
	vposition = vertex_position;
	vcolor = vertex_color;
	plighting = particle_lighting;
	pblur = particle_blur;
}
