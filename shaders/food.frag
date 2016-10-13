#version 410

in VS_OUT {
	vec3 fN;
	vec3 fV;
	vec3 fL;
} fs_in;
out vec4 frag_color;

void main(){
	vec3 ambient = vec3(0.0,0.0,0.0);
	vec3 diffuse = vec3(0.8,0.6,0.2);
	vec3 specular = vec3(0.3,0.3,0.3);
	float shininess = 100;
	vec3 N = normalize(fs_in.fN);
	vec3 V = normalize(fs_in.fV);
	vec3 L = normalize(fs_in.fL);
	vec3 R = normalize(reflect(-L,N));
	vec3 dif = diffuse*max(dot(L,N),0.0);
	vec3 spec = specular*pow(max(dot(V,R),0.0),shininess);
	frag_color.xyz = ambient + dif + spec;
	frag_color.w = 1.0;
}
