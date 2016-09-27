#version 410
//vert

layout (location = 0) vec3 vertex;
layout (location = 2) vec3 texCord;
uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform vertex_world_to_clip;
out VS_OUT{
	vec3 texCord;
} vs_out;

void main(){
	glPosition = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}		
