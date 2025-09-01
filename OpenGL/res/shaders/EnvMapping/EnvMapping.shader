#shader vertex
#version 330 
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform mat4 norm_martix;

out VS_OUT{
    vec3 posVS;
    vec3 nomralVS;
} vs_out;

void main()
{
    vec4 posVS = mv_matrix * vec4(position,1.0);
    vs_out.posVS = posVS.xyz;
    vs_out.nomralVS = (norm_martix * vec4(normal,0.0)).xyz;
    gl_Position = proj_matrix * posVS;
}


#shader fragment
#version 330 core
in VS_OUT{
    vec3 posVS;
    vec3 normalVS;
} fs_in;

uniform samplerCube EnvironmentTex;

out vec4 FragColor;

void main()
{
    vec3 N = normalize(fs_in.normalVS);
    vec3 I = normalize(fs_in.posVS);
    vec3 R = reflect(I,N);

    vec3 color = texture(EnvironmentTex,R).rgb;
    FragColor = vec4(color,1.0);

}