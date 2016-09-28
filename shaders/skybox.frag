#version 410
//frag

uniform samplerCube cubeMapName;

in VS_OUT{
	vec3 fN;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 N = normalize(fs_in.fN);
	frag_color = texture(cubeMapName,N);
}
