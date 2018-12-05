#version 430 core
#extension GL_NV_shader_thread_group : require

layout(binding = 0) uniform sampler3D tex;
layout(location = 0) uniform vec2 screenSize;

out vec4 fragColor;

void main()
{
	vec3 coord3;
	coord3.xy = gl_FragCoord.st / screenSize;
	coord3.z = 0;

	vec4 color = texture(tex, coord3);
	fragColor = color;
}