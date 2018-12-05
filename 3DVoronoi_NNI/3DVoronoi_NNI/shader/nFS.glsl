#version 430 core
#extension GL_NV_shader_thread_group : require

#define M_PI	3.141592
#define M_PI_2	1.570796

layout(binding = 0) uniform sampler3D tex;
layout(location = 0) uniform vec2 screenSize;
layout(location = 1) uniform vec2 eye;
layout(location = 2) uniform vec3 volume;
layout(binding = 2) uniform Camera{ // 144
	vec3 origin;   // 0   //12
	mat4 pmat;      // 16   //64
	mat4 vmat;      // 80   //64
};

out vec4 fragColor;

struct Ray {
	vec3 o;
	vec3 d;
	vec3 nearPos;
};

Ray createRay(vec2 st)
{
	vec2 adjust = (st / screenSize);
	mat4 inv = inverse(pmat * vmat);
	vec4 tmp = vec4(adjust - vec2(0.5f), 0.0f, 1.0f);
	tmp = inv * tmp;
	tmp.xyz /= tmp.w;
	vec3 nearPos = tmp.xyz;

	Ray ray;
	ray.o = origin;
	ray.d = normalize(nearPos - origin);
	ray.nearPos = nearPos;

	return ray;
}

void main()
{
	float t = 0.01f;
	Ray ray = createRay(gl_FragCoord.st);
	vec4 color = vec4(0.0f);
	vec3 coord3D = ray.o;
	vec3 step = ray.d * t;

	vec4 sum = vec4(0.0f);
	for (int i = 0; i < 1000; i++)
	{
		color = texture(tex, coord3D);
		float intencity = color.r;
		//color = vec4(cos(color.r * M_PI_2 - M_PI_2), sin(color.g * M_PI), cos(color.r * M_PI_2), 0.02f);
		color = vec4(cos(intencity * M_PI_2 - M_PI_2), sin(intencity * M_PI), cos(intencity * M_PI_2 - M_PI_2), intencity);
		color.r *= intencity;
		color.g *= intencity;
		color.b *= intencity;
		//sum = sum + color * (1.0f - sum.w);
		sum = mix(sum, color, (1.0f - intencity));
		coord3D += step;
	}

	fragColor = sum;
}