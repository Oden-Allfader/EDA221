#version 410
//vert

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 camera_position;
out VS_OUT{
	vec3 fN;
} vs_out;

void main(){
	//vs_out.fN = (normal_model_to_world * vec4(normal,0.0)).xyz;
	vs_out.fN = (vertex_model_to_world * vec4(vertex,1.0)).xyz;
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex + camera_position, 1.0);
}
