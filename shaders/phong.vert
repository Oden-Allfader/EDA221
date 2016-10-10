#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 light_position;
uniform vec3 camera_position;

out VS_OUT {
	vec3 fN;
	vec3 fV;
	vec3 fL;
}vs_out;

void main(){
	vec3 worldPos = (vertex_model_to_world * vec4(vertex,1.0)).xyz;
	vs_out.fN = (normal_model_to_world * vec4(normal,0.0)).xyz;
	vs_out.fV = camera_position - worldPos;
	vs_out.fL = light_position - worldPos;
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
