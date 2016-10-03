#version 410
//vert

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform vec3 light_position;
uniform vec3 camera_position;
uniform float time;

out VS_OUT{
	vec3 fN;
	vec3 fT;
	vec3 fB;
	vec2 fTex;
	vec3 fV;
	vec3 fL;
} vs_out;


void main(){

	float amplitude = 1.0;
	vec3 direction = vec3(0.7,0.0,0.7);
	float freq = 0.4;
	float phase = 0.5;
	float k = 2.0;

	vec3 worldPos = (vertex_model_to_world * vec4(vertex,1.0)).xyz;
	vs_out.fN = normalize((normal_model_to_world * vec4(normal,1.0)).xyz);
	vs_out.fT = normalize((normal_model_to_world * vec4(tangent,1.0)).xyz);
	vs_out.fB = normalize((normal_model_to_world * vec4(binormal,1.0)).xyz);
	vs_out.fV = camera_position - worldPos;
	vs_out.fL = light_position - worldPos;
	vs_out.fTex = vec2(texcoord.x,texcoord.y);
	vec4 outVec1 = vec4(vertex, 1.0);
	outVec1.y = amplitude * pow((sin((direction.x*outVec1.x + direction.z*outVec1.z)*freq + time*phase) * 0.5 + 0.5),k);
	vec4 outVec2 = vec4(vertex, 1.0);
	outVec2.y = 1.0 * pow((sin((1.0*outVec2.x + 0*outVec2.z)*0.2 + time*0.3) * 0.5 + 0.5),4);
	gl_Position = vertex_world_to_clip * vertex_model_to_world * (outVec1 + outVec2);
}
