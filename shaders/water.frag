#version 410
//frag
uniform sampler2D bumpTex;
uniform samplerCube cubeTex;
uniform float time;

in VS_OUT{
	vec3 fN; //Normal
	vec3 fT; //Tangent
	vec3 fB; //Binormal
	vec2 fTex; //Texturecoord
	vec3 fV; //View Vector
	vec3 fL; // Light vector
} fs_in;

out vec4 frag_color;

void main(){
	vec3 N = normalize(fs_in.fN);
	vec3 T = normalize(fs_in.fT);
	vec3 B = normalize(fs_in.fB);
	vec3 V = normalize(fs_in.fV);
	vec3 L = normalize(fs_in.fL);
	vec3 texCoord;
	texCoord.xy = fs_in.fTex;
	texCoord.z = 0;
	//Bumping
	vec2 texScale = vec2(8.0,4.0);
	float bumpTime = mod(time,0.1);
	vec2 bumpSpeed = vec2(-0.05,0);
	vec2 bumpCoord0 = fs_in.fTex*texScale + bumpTime*bumpSpeed;
	vec2 bumpCoord1 = fs_in.fTex*texScale*2 + bumpTime*bumpSpeed*4;
	vec2 bumpCoord2 = fs_in.fTex*texScale*4 + bumpTime*bumpSpeed*8;
	vec3 n0 = 2.0 * texture(bumpTex,bumpCoord0).rgb - 1;
	vec3 n1 = 2.0 * texture(bumpTex,bumpCoord1).rgb - 1;
	vec3 n2 = 2.0 * texture(bumpTex,bumpCoord2).rgb - 1;
	vec3 normal = normalize(n0 + n1 + n2);
	mat3 vector_transform;
	vector_transform[0] = T;
	vector_transform[1] = B;
	vector_transform[2] = N;
	normal = normalize(vector_transform * normal);
	
	// Coloring
	vec4 colorDeep = vec4(0.0,0.0,0.1,1.0);
	vec4 colorShallow = vec4(0.0,0.5,0.5,1.0);
	float facing = 1 - max(dot(V,N),0);

	vec3 R = normalize(reflect(-V,N));
	vec4 reflection = texture(cubeTex,R);
//	vec4 texColor = texture(thisTex,fs_in.fTex);
//	float dif = max(dot(normal,L),0.0);
//	vec3 spec = specular*pow(max(dot(V,R),0.0),shininess);
	vec4 waterColor = mix(colorDeep,colorShallow,facing);
	frag_color = waterColor + reflection;
}
