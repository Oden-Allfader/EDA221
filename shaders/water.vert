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
	vec3 sN;
	vec3 sT;
	vec3 sB;
} vs_out;


void main(){
	//first wave
	float a1 = 1.0;
	float f1 = 0.1;
	float p1 = 0.5;
	float k1 = 2.0;
	vec3 dir1 = vec3(-1.0,0.0,0.0);

	//second wave
	float a2 = 0.5;
	float f2 = 0.2;
	float p2 = 1.3;
	float k2 = 4.0;
	vec3 dir2 = vec3(-0.7,0.0,0.7);

	vec3 worldPos = (vertex_model_to_world * vec4(vertex,1.0)).xyz;
	vs_out.sN = (vec4(normal,1.0)).xyz;
	vs_out.sT = (vec4(tangent,1.0)).xyz;
	vs_out.sB = (vec4(binormal,1.0)).xyz;
	vs_out.fV = camera_position - worldPos;
	vs_out.fL = light_position - worldPos;
	vs_out.fTex = vec2(texcoord.x,texcoord.y);
	
	vec4 g1 = vertex_model_to_world * vec4(vertex, 1.0);
	g1.y = g1.y + a1 * pow((sin((dir1.x*g1.x + dir1.z*g1.z)*f1 + time*p1) * 0.5 + 0.5),k1);
	float dg1x = 0.5*k1*f1*a1*pow((sin((dir1.x * g1.x + dir1.z * g1.z)*f1 + time*p1)*0.5 + 0.5),k1-1)*cos((dir1.x*g1.x + dir1.z * g1.z)*f1 + time * p1) * dir1.x;
	float dg1z = 0.5*k1*f1*a1*pow((sin((dir1.x * g1.x + dir1.z * g1.z)*f1 + time*p1)*0.5 + 0.5),k1-1)*cos((dir1.x*g1.x + dir1.z * g1.z)*f1 + time * p1) * dir1.z;

	vec4 g2 = vertex_model_to_world * vec4(vertex, 1.0);
	g2.y = g2.y + a2 * pow((sin((dir2.x*g2.x + dir2.z*g2.z)*f2 + time*p2) * 0.5 + 0.5),k2);
	float dg2x = 0.5*k2*f2*a2*pow((sin((dir2.x * g2.x + dir2.z * g2.z)*f2 + time*p2)*0.5 + 0.5),k2-1)*cos((dir2.x*g2.x + dir2.z * g2.z)*f2 + time * p2) * dir2.x;
	float dg2z = 0.5*k2*f2*a2*pow((sin((dir2.x * g2.x + dir2.z * g2.z)*f2 + time*p2)*0.5 + 0.5),k2-1)*cos((dir2.x*g2.x + dir2.z * g2.z)*f2 + time * p2) * dir2.z;

	vec4 h = g1 + g2;
	float dhdx = dg1x + dg2x;
	float dhdz = dg1z + dg2z;
	vs_out.fN = vec3(-dhdx,1,-dhdz);
	vs_out.fB = vec3(1.0,dhdx,0.0);
	vs_out.fT = vec3(0.0,dhdz,1.0);
 

	gl_Position = vertex_world_to_clip * h;
}
