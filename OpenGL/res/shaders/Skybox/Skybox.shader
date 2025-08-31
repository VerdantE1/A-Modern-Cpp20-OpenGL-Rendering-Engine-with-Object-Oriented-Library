#shader vertex
#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex; // unused

out vec3 vDir;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Use cube vertex as lookup direction
    vDir = position;

    // Project skybox and force depth to 1.0 to avoid clipping (works with GL_LEQUAL)
    vec4 pos = projection * view * vec4(position, 1.0);
    gl_Position = pos.xyww;
}

#shader fragment
#version 430

in vec3 vDir;
out vec4 fragColor;

uniform samplerCube skybox;

void main()
{
    fragColor = texture(skybox, normalize(vDir));
}