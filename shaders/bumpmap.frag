#version 410
//frag
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;
uniform sampler2D myBumpMap;
uniform sampler2D thisTex;
uniform mat4 normal_model_to_world;
in VS_OUT{
	vec3 fN;
	vec3 fT;
	vec3 fB;
	vec2 fTex;
	vec3 fV;
	vec3 fL;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 N = normalize(fs_in.fN);
	vec3 T = normalize(fs_in.fT);
	vec3 B = normalize(fs_in.fB);
	vec3 V = normalize(fs_in.fV);
	vec3 L = normalize(fs_in.fL);
	vec3 normal = 2.0 * texture(myBumpMap,fs_in.fTex).rgb - 1;
	normal = normalize(normal);
	mat3 vector_transform;
	vector_transform[0] = T;
	vector_transform[1] = B;
	vector_transform[2] = N;
	normal = vector_transform * normal;
	
	vec3 R = normalize(reflect(-L,normal));
	vec4 texColor = texture(thisTex,fs_in.fTex);
	float dif = max(dot(normal,L),0.0);
	vec3 spec = specular*pow(max(dot(V,R),0.0),shininess);
	frag_color.xyz = ambient + (dif * texColor.rgb) + spec ;
	frag_color.w = 0;
}
