#version 410
//frag

uniform samplerCube cubeMapName;

in VS_OUT{
	vec3 texCord;
} fs_in;
out vec4 frag_color;

void main(){
	frag_color = texture(cubeMapName,texCord);
}
