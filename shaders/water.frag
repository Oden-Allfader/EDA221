#version 410
//frag
uniform sampler2D bumpTex;
uniform samplerCube cubeTex;
uniform float time;
uniform mat4 normal_model_to_world;

in VS_OUT{
	vec3 fN; //Normal
	vec3 fT; //Tangent
	vec3 fB; //Binormal
	vec2 fTex; //Texturecoord
	vec3 fV; //View Vector
	vec3 fL; // Light vector
	vec3 sN; // surface -> world, maybe to be used later
	vec3 sT;//
	vec3 sB;//
} fs_in;

out vec4 frag_color;

void main(){
	vec3 N = normalize(fs_in.fN);
	vec3 T = normalize(fs_in.fT);
	vec3 B = normalize(fs_in.fB);
	vec3 V = normalize(fs_in.fV);
	vec3 L = normalize(fs_in.fL);
	vec3 surfN = normalize(fs_in.sN);
	vec3 surfT = normalize(fs_in.sT);
	vec3 surfB = normalize(fs_in.sB);
	float r0 = 0.02037;

	vec3 texCoord;
	texCoord.xz = fs_in.fTex;
	texCoord.y = 0.0;

	//Bumping
	vec2 texScale = vec2(8.0,4.0);
	float bumpTime = mod(time,0.1);
	vec2 bumpSpeed = vec2(-0.05,0.0);
	vec2 bumpCoord0 = fs_in.fTex*texScale + bumpTime*bumpSpeed*0.01;
	vec2 bumpCoord1 = fs_in.fTex*texScale*2 + bumpTime*bumpSpeed*0.04;
	vec2 bumpCoord2 = fs_in.fTex*texScale*4 + bumpTime*bumpSpeed*0.08;
	vec3 n0 = 2.0 * texture(bumpTex,bumpCoord0).rgb - 1;
	vec3 n1 = 2.0 * texture(bumpTex,bumpCoord1).rgb - 1;
	vec3 n2 = 2.0 * texture(bumpTex,bumpCoord2).rgb - 1;
	vec3 normal = normalize(n0 + n1 + n2);
	mat3 vector_transform;
	vector_transform[0] = B;
	vector_transform[1] = T;
	vector_transform[2] = N;
	//mat3 model_transform; surface -> world, maybe to be used later
	//model_transform[0] = surfT;
	//model_transform[1] = surfB;
	//model_transform[2] = surfN;
	//normal = normalize((normal_model_to_world * vec4(model_transform * vector_transform * normal,1.0)).xyz);
	normal = normalize( vector_transform * normal);
	
	// Coloring
	vec4 colorDeep = vec4(0.0,0.0,0.1,1.0);
	vec4 colorShallow = vec4(0.0,0.5,0.5,1.0);
	float facing = 1 - max(dot(V,normal),0.0);
	vec4 waterColor = mix(colorDeep,colorShallow,facing);

	//Reflection
	vec3 R = (reflect(-V,normal));
	vec4 reflection = clamp(vec4(texture(cubeTex,R)),0.0,1.0);
	float fresnel = r0 + (1.0 - r0)*pow((1-dot(V,normal)),5.0);

	//Refraction
	vec4 refraction = clamp(vec4(texture(cubeTex,refract(V,normal,1/1.33))),0.0,1.0);

	frag_color = waterColor +  (reflection * fresnel) + (refraction * (1 - fresnel));

}
